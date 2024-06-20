#include "../include/vkfs_rocksdb.h"
#include "../include/vkfs_utils.h"
#include "rocksdb/db.h"
#include "rocksdb/cache.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

VKFSRocksDB* VKFSRocksDB::SingletonRocksDB = nullptr;

VKFSRocksDB::VKFSRocksDB() : db(nullptr) {}

VKFSRocksDB::~VKFSRocksDB() {
    if (db) {
        delete db;
        db = nullptr;
    }
}

VKFSRocksDB* VKFSRocksDB::getInstance() {
    if (SingletonRocksDB == nullptr) {
        SingletonRocksDB = new VKFSRocksDB();
    }
    return SingletonRocksDB;
}

void VKFSRocksDB::initOptions(){

    //LRU Cache 1gb
    size_t cache_capacity = 1024 * 1024 * 1024;
    std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(cache_capacity);
    tableOptions.block_cache = cache;

    //Recommended settings from RocksDB Wiki
    options.bytes_per_sync = 1048576;
    options.compaction_pri = rocksdb::kMinOverlappingRatio;
    tableOptions.block_size = 16 * 1024;
    tableOptions.cache_index_and_filter_blocks = true;
    tableOptions.pin_l0_filter_and_index_blocks_in_cache = true;
    options.create_if_missing = true;

    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tableOptions));
    tableOptions.optimize_filters_for_memory = true;

    //Enable WAL
    writeOptions.disableWAL = false;

}

int VKFSRocksDB::initDatabase(const std::string& dataBaseDirectory) {
    if (db) {
        printf("Database already initialized\n");
        return -1;
    }

    VKFSRocksDB::initOptions();

    int ret;
    namespace fs = std::filesystem;
    std::string projectPath = vkfs::path::getProjectRoot();
    std::string dbPath = projectPath + "/" + dataBaseDirectory;
    std::filesystem::remove_all(dbPath);

    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &db);

    if (!status.ok()) {
        printf("Failed to open Database: %s\n", status.ToString().c_str());
        ret = -1;
    } else {
        printf("Database opened successfully\n");
        ret = 0;
    }

    return ret;
}

void VKFSRocksDB::cleanup(){
    if (db) {
        delete db;
        db = nullptr;
    }
}

void VKFSRocksDB::printStatusErr(rocksdb::Status status){
    std::cout << "RocksDB error: " << status.ToString() << std::endl;
}

int VKFSRocksDB::get(const VKFS_KEY key, std::string& value) {
    rocksdb::Status status = db->Get(readOptions, std::to_string(key), &value);
    if (!status.ok()) {
        #ifdef DEBUG
            HFSRocksDB::printStatusErr(status);
        #endif
        return -1;
    }
    return 0;
}

int VKFSRocksDB::put(const VKFS_KEY key, const VKFSHeaderSerialized& value) {
    rocksdb::Status status = db->Put(writeOptions, std::to_string(key), getValueSlice(value));
    if (!status.ok()) {
        #ifdef DEBUG
            HFSRocksDB::printStatusErr(status);
        #endif
        return -1;
    }
    return 0;
}

int VKFSRocksDB::remove(const VKFS_KEY key){
    rocksdb::Status status = db->Delete(writeOptions, std::to_string(key));
    if (!status.ok()) {
        #ifdef DEBUG
            HFSRocksDB::printStatusErr(status);
        #endif
        return -1;
    }
    return 0;
}

rocksdb::Slice VKFSRocksDB::getValueSlice(const VKFSHeaderSerialized& value) {
    return rocksdb::Slice(value.data, value.size);
}

struct stat VKFSRocksDB::getMetaData(VKFS_KEY key) {
    std::string data;
    VKFSRocksDB::get(key,data);

    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if(*flag){
        VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
        return metaData->file_structure;

    }else{
        VKFSFileMetaData* metaData = reinterpret_cast<VKFSFileMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
        return metaData->file_structure;
    }
}

int VKFSRocksDB::updateMetaData(VKFS_KEY key, struct stat new_stat) {
    std::string data;
    if(VKFSRocksDB::get(key,data) < 0){
        return -1;
    }
    
    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if(!*flag){ //File
        VKFSFileMetaData* metaData = reinterpret_cast<VKFSFileMetaData*>(headerValue.data + sizeof(VKFS_FILE_FLAG));
        metaData->file_structure = new_stat;
    } else{ //Directory
        VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(headerValue.data + sizeof(VKFS_DIR_FLAG));
        metaData->file_structure = new_stat;
    }

    return VKFSRocksDB::put(key,headerValue);
}

std::vector<VKFS_KEY> VKFSRocksDB::getDirEntries(VKFS_KEY key){
    std::string data;
    VKFSRocksDB::get(key,data);

    VKFSHeaderSerialized dirHeader;
    dirHeader.data = &data[0];
    dirHeader.size = data.size();

    VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(dirHeader.data + VKFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;
    std::vector<VKFS_KEY> dirEntries(noOfEntries);
    char* keyBuffer = dirHeader.data + VKFS_FLAG_SIZE + VKFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
    VKFS_KEY* entryKey = nullptr;
    for(int i = 0;i < noOfEntries;i++){
        entryKey = reinterpret_cast<VKFS_KEY*>(keyBuffer);
        dirEntries[i] = *entryKey;
        keyBuffer += sizeof(VKFS_KEY);
    }

    return dirEntries;
}

std::string VKFSRocksDB::getFilename(VKFS_KEY key) {
    std::string data;
    VKFSRocksDB::get(key,data);

    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if (*flag) {
        VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
        char* filenameBuffer = headerValue.data + VKFS_FLAG_SIZE + VKFS_DIR_HEADER_SIZE;
        return std::string(filenameBuffer, metaData->filename_len);
    } else {
        VKFSFileMetaData* metaData = reinterpret_cast<VKFSFileMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
        char* filenameBuffer = headerValue.data + VKFS_FLAG_SIZE + VKFS_FILE_HEADER_SIZE;
        return std::string(filenameBuffer, metaData->filename_len);
    }

}

int VKFSRocksDB::incrementDirLinkCount(VKFS_KEY parentKey, VKFS_KEY key){
    std::string data;
    if(VKFSRocksDB::get(parentKey,data) < 0){
        return -1;
    }

    VKFSHeaderSerialized currentHeader;
    currentHeader.data = &data[0];
    currentHeader.size = data.size();

    VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(currentHeader.data + sizeof(VKFS_DIR_FLAG));

    uint currentEntries = metaData->file_structure.st_nlink - 2; //Itself and "." entry not accounted for
    metaData->file_structure.st_nlink++;

    VKFSHeaderSerialized newHeader;
    newHeader.size = data.size() + sizeof(VKFS_KEY);
    newHeader.data = new char[newHeader.size];
    memcpy(newHeader.data,currentHeader.data,data.size()); //Copy of existing data


    char* entriesPtr = newHeader.data + VKFS_FLAG_SIZE + VKFS_DIR_HEADER_SIZE + metaData->filename_len + 1 + (currentEntries*sizeof(VKFS_KEY));

    memcpy(entriesPtr,&key,sizeof(VKFS_KEY)); //Insert new entry
    int ret =  VKFSRocksDB::put(parentKey,newHeader);

    delete[] newHeader.data;

    return ret;
}

int VKFSRocksDB::deleteDirEntry(VKFS_KEY parentKey, VKFS_KEY key){
    std::string data;
    if(VKFSRocksDB::get(parentKey,data) < 0){
        return -1;
    }

    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();    
    VKFSDirMetaData* metaData = reinterpret_cast<VKFSDirMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;

    std::vector<VKFS_KEY> dirEntries;
    dirEntries.reserve(noOfEntries - 1);
    metaData->file_structure.st_nlink--;
    char* keyBuffer = headerValue.data + VKFS_FLAG_SIZE + VKFS_DIR_HEADER_SIZE + metaData->filename_len + 1;

    if(noOfEntries > 1){
        VKFS_KEY* entryKey = nullptr;
        for(uint32_t i = 0;i < noOfEntries;i++){
            entryKey = (VKFS_KEY*)keyBuffer;
            if(*entryKey == key){
                keyBuffer += sizeof(VKFS_KEY);
                continue;
            }
            //dirEntries[i] = *entryKey;
            dirEntries.push_back(*entryKey);
            keyBuffer += sizeof(VKFS_KEY);
        }
        noOfEntries--;
        keyBuffer = headerValue.data + VKFS_FLAG_SIZE + VKFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
        memcpy(keyBuffer, dirEntries.data(),noOfEntries*sizeof(VKFS_KEY));
    }

    headerValue.size = data.size() - sizeof(VKFS_KEY);

    return VKFSRocksDB::put(parentKey,headerValue);
}

int VKFSRocksDB::setBlob(VKFS_KEY key){
    std::string data;
    if(VKFSRocksDB::get(key,data) < 0){
        return -1;
    }

    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();
    struct VKFSFileMetaData* inode_value = reinterpret_cast<struct VKFSFileMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
    inode_value->has_external_data = true;

    return VKFSRocksDB::put(key,headerValue);
}

int VKFSRocksDB::setNewFileSize(VKFS_KEY key,size_t size){
    std::string data;
    if(VKFSRocksDB::get(key,data) < 0){
        return -1;
    }

    VKFSHeaderSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();
    struct VKFSFileMetaData* metaData = reinterpret_cast<struct VKFSFileMetaData*>(headerValue.data + VKFS_FLAG_SIZE);
    metaData->file_structure.st_size = size;

    return VKFSRocksDB::put(key,headerValue);
}

VKFSFileMetaData VKFSRocksDB::getFileHeader(VKFS_KEY key){
    std::string data;
    VKFSRocksDB::get(key,data);
    VKFSFileMetaData headerValue;
    memcpy(&headerValue,data.data() + VKFS_FLAG_SIZE,VKFS_FILE_HEADER_SIZE);

    return headerValue;

}
