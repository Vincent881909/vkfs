#include "../include/hfs_rocksdb.h"
#include "../include/hfs_utils.h"
#include "../include/hfs_inode.h"
#include "rocksdb/db.h"
#include "rocksdb/cache.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include <filesystem>
#include <iostream>
#include <filesystem>

HFSRocksDB* HFSRocksDB::SingletonRocksDB = nullptr;

HFSRocksDB::HFSRocksDB() : db(nullptr) {}

HFSRocksDB::~HFSRocksDB() {
    if (db) {
        delete db;
        db = nullptr;
    }
}

HFSRocksDB* HFSRocksDB::getInstance() {
    if (SingletonRocksDB == nullptr) {
        SingletonRocksDB = new HFSRocksDB();
    }
    return SingletonRocksDB;
}

void HFSRocksDB::initOptions(){

    //This needs fixing and more research

    size_t cache_capacity = 1024 * 1024 * 1024; // 1 GB

    // Create a shared pointer to a new LRU cache with the specified capacity
    std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(cache_capacity);
    tableOptions.block_cache = cache;


    // Apply the BlockBasedTableOptions to RocksDB Options
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tableOptions));

    //Recommended settings from RocksDB Wiki
    options.bytes_per_sync = 1048576;
    options.compaction_pri = rocksdb::kMinOverlappingRatio;
    tableOptions.block_size = 16 * 1024;
    tableOptions.cache_index_and_filter_blocks = true;
    tableOptions.pin_l0_filter_and_index_blocks_in_cache = true;
    options.create_if_missing = true;

    //Setup BloomFilter
    int bloom_filter_bits_per_key = 10; // Number of bits used for each key in the Bloom filter
    tableOptions.filter_policy.reset(rocksdb::NewBloomFilterPolicy(bloom_filter_bits_per_key));

    //Assign BlockBasedTableOptions to the DB options
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tableOptions));
    tableOptions.optimize_filters_for_memory = true;

    writeOptions.disableWAL = false;

}

void HFSRocksDB::initReadOptions(){
    //TODO
}

void HFSRocksDB::initWriteOptions(){
    //TODO
}

int HFSRocksDB::initDatabase(const std::string& dataBaseDirectory) {
    if (db) {
        printf("Database already initialized\n");
        return -1;
    }

    HFSRocksDB::initOptions();
    HFSRocksDB::initReadOptions();
    HFSRocksDB::initWriteOptions();

    int ret;
    namespace fs = std::filesystem;
    std::string projectPath = hfs::path::getProjectRoot();
    std::string dbPath = projectPath + "/" + dataBaseDirectory;
    std::filesystem::remove_all(dbPath);

    // Open the database
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

void HFSRocksDB::cleanup(){
    if (db) {
        delete db;
        db = nullptr;
    }
}

void HFSRocksDB::printStatusErr(rocksdb::Status status){
    std::cout << "Operation successful: " << status.ToString() << std::endl;
}

int HFSRocksDB::get(const HFS_KEY key, std::string& value) {
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), std::to_string(key), &value);
    if (!status.ok()) {
        #ifdef DEBUG
            HFSRocksDB::printStatusErr(status);
        #endif
        return -1;
    }
    return 0;
}

int HFSRocksDB::put(const HFS_KEY key, const HFSInodeValueSerialized& value) {
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), getValueSlice(value));
    if (!status.ok()) {
        #ifdef DEBUG
            HFSRocksDB::printStatusErr(status);
        #endif
        return -1;
    }
    return 0;
}

int HFSRocksDB::remove(const HFS_KEY key){
    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), std::to_string(key));
    if (!status.ok()) {
        return -1;
    }
    return 0;
}

rocksdb::Slice HFSRocksDB::getValueSlice(const HFSInodeValueSerialized& value) {
    return rocksdb::Slice(value.data, value.size);
}

struct stat HFSRocksDB::getMetaData(HFS_KEY key) {
    std::string data;
    HFSRocksDB::get(key,data);

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if(*flag){
        HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + HFS_FLAG_SIZE);
        return metaData->file_structure;

    }else{
        HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
        return metaData->file_structure;
    }
}

int HFSRocksDB::updateMetaData(HFS_KEY key, struct stat new_stat) {
    std::string data;
    if(HFSRocksDB::get(key,data) < 0){
        return -1;
    }
    
    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if(!*flag){ //File
        HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headerValue.data + sizeof(HFS_FILE_FLAG));
        metaData->file_structure = new_stat;
    } else{ //Directory
        HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + sizeof(HFS_DIR_FLAG));
        metaData->file_structure = new_stat;
    }

    return HFSRocksDB::put(key,headerValue);
}

std::vector<HFS_KEY> HFSRocksDB::getDirEntries(HFS_KEY key){
    std::string data;
    HFSRocksDB::get(key,data);

    HFSInodeValueSerialized dirHeader;
    dirHeader.data = &data[0];
    dirHeader.size = data.size();

    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(dirHeader.data + HFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;
    std::vector<HFS_KEY> dirEntries(noOfEntries);
    char* keyBuffer = dirHeader.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
    HFS_KEY* entryKey = nullptr;
    for(int i = 0;i < noOfEntries;i++){
        entryKey = reinterpret_cast<HFS_KEY*>(keyBuffer);
        dirEntries[i] = *entryKey;
        keyBuffer += sizeof(HFS_KEY);
    }

    return dirEntries;
}

std::string HFSRocksDB::getFilename(HFS_KEY key) {
    std::string data;
    HFSRocksDB::get(key,data);

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

    if (*flag) {
        HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + HFS_FLAG_SIZE);
        char* filenameBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE;
        return std::string(filenameBuffer, metaData->filename_len);
    } else {
        HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
        char* filenameBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE;
        return std::string(filenameBuffer, metaData->filename_len);
    }

}

int HFSRocksDB::incrementDirLinkCount(HFS_KEY parentKey, HFS_KEY key){
    std::string data;
    if(HFSRocksDB::get(parentKey,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized currentHeader;
    currentHeader.data = &data[0];
    currentHeader.size = data.size();

    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(currentHeader.data + sizeof(HFS_DIR_FLAG));

    uint currentEntries = metaData->file_structure.st_nlink - 2; //Itself and "." entry not accounted for
    metaData->file_structure.st_nlink++;

    HFSInodeValueSerialized newHeader;
    newHeader.size = data.size() + sizeof(HFS_KEY);
    newHeader.data = new char[newHeader.size];
    memcpy(newHeader.data,currentHeader.data,data.size()); //Copy of existing data


    char* entriesPtr = newHeader.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1 + (currentEntries*sizeof(HFS_KEY));

    memcpy(entriesPtr,&key,sizeof(HFS_KEY)); //Insert new entry
    int ret =  HFSRocksDB::put(parentKey,newHeader);

    delete[] newHeader.data;

    return ret;
}

int HFSRocksDB::deleteDirEntry(HFS_KEY parentKey, HFS_KEY key){
    std::string data;
    if(HFSRocksDB::get(parentKey,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();    
    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + HFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;

    std::vector<HFS_KEY> dirEntries;
    dirEntries.reserve(noOfEntries - 1);
    metaData->file_structure.st_nlink--;
    char* keyBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;

    if(noOfEntries > 1){
        HFS_KEY* entryKey = nullptr;
        for(uint32_t i = 0;i < noOfEntries;i++){
            entryKey = (HFS_KEY*)keyBuffer;
            if(*entryKey == key){
                keyBuffer += sizeof(HFS_KEY);
                continue;
            }
            //dirEntries[i] = *entryKey;
            dirEntries.push_back(*entryKey);
            keyBuffer += sizeof(HFS_KEY);
        }
        noOfEntries--;
        keyBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
        memcpy(keyBuffer, dirEntries.data(),noOfEntries*sizeof(HFS_KEY));
    }

    headerValue.size = data.size() - sizeof(HFS_KEY);

    return HFSRocksDB::put(parentKey,headerValue);
}

int HFSRocksDB::setBlob(HFS_KEY key){
    std::string data;
    if(HFSRocksDB::get(key,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();
    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
    inode_value->has_external_data = true;

    return HFSRocksDB::put(key,headerValue);
}

int HFSRocksDB::setNewFileSize(HFS_KEY key,size_t size){
    std::string data;
    if(HFSRocksDB::get(key,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();
    struct HFSFileMetaData* metaData = reinterpret_cast<struct HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
    metaData->file_structure.st_size = size;

    return HFSRocksDB::put(key,headerValue);
}

HFSFileMetaData HFSRocksDB::getFileHeader(HFS_KEY key){
    std::string data;
    HFSRocksDB::get(key,data);
    HFSFileMetaData headerValue;
    memcpy(&headerValue,data.data() + HFS_FLAG_SIZE,HFS_FILE_HEADER_SIZE);

    return headerValue;

}
