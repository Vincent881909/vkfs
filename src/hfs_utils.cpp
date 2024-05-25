#include "../include/hfs_utils.h"
#include "../include/hash.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace hfs {
namespace inode {

HFS_KeyHandler* getKeyHandler(struct fuse_context* context){
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    return hfsState->getKeyHandler();

}

void prepareWrite(HFSInodeValueSerialized& inodeData, const char* buf, size_t size, char*& newData) {
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data + HFS_FLAG_SIZE);
    inodeValue->file_structure.st_size = size;

    if (inodeValue->file_structure.st_size == 0 && size == 0) {
        return;
    }

    size_t memoryNeeded = HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1 + size;
    newData = new char[memoryNeeded];

    memcpy(newData, inodeData.data, memoryNeeded - size); // Copy metadata
    char* dataBuf = newData + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1;
    memcpy(dataBuf, buf, size);

    inodeData.data = newData;
    inodeData.size = memoryNeeded;
}

int writeToKeyValue(rocksdb::DB* db, HFS_KEY key,size_t size,const char* sourceBuf){
    int ret = 0;

    HFSInodeValueSerialized keyValue;
    keyValue.data = nullptr;
    keyValue.data = 0;
    hfs::db::getSerializedData(db,key,keyValue);
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(keyValue.data + HFS_FLAG_SIZE);

    if(inodeValue->file_structure.st_size == 0 && !size){ //Nothing do be done
        return 0;
    }

    inodeValue->file_structure.st_size = size;

    size_t memoryNeeded = HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1 + size ;
    char* newData = new char[memoryNeeded];

    memcpy(newData,keyValue.data,memoryNeeded - size); //Copy metadata
    char* dataBuf = newData + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1;
    memcpy(dataBuf,sourceBuf,size);

    delete[] keyValue.data;
    keyValue.data = newData;
    keyValue.size = memoryNeeded;

    ret = hfs::db::rocksDBInsert(db,key,keyValue);
 
    delete[] newData;

    return ret;

}

void initStat(struct stat &statbuf, mode_t mode) {
    statbuf.st_mode = mode;

    if (S_ISREG(mode)) {
        statbuf.st_nlink = 1;
    } else if (S_ISDIR(mode)) {
        printf("Init stat is dir\n");
        statbuf.st_nlink = 2;
    }

    statbuf.st_uid = 0;
    statbuf.st_gid = 0;
    statbuf.st_rdev = 0;
    statbuf.st_size = 0;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    statbuf.st_atim = now;
    statbuf.st_mtim = now;
    statbuf.st_ctim = now;
    statbuf.st_blksize = 0;
    statbuf.st_blocks = 0;
}

HFSInodeValueSerialized initDirHeader(struct stat fileStructure, std::string filename, mode_t mode){
    struct HFSInodeValueSerialized dirHeaderValue;

    dirHeaderValue.size = HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + filename.length() + 1 ;
    dirHeaderValue.data = new char[dirHeaderValue.size];

    memcpy(dirHeaderValue.data,&HFS_DIR_FLAG,HFS_FLAG_SIZE);

    struct HFSDirMetaData* dirHeader = reinterpret_cast<struct HFSDirMetaData*>(dirHeaderValue.data + HFS_FLAG_SIZE);
    initStat(fileStructure, S_IFDIR | mode);
    dirHeader->file_structure = fileStructure;
    dirHeader->filename_len = filename.length();

    char* name_buf = dirHeaderValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());
    name_buf[dirHeader->filename_len] = '\0';

    return dirHeaderValue;

}

HFSInodeValueSerialized initFileHeader(struct stat fileStructure, std::string filename, mode_t mode) {
    struct HFSInodeValueSerialized inode_data;

    inode_data.size = HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    memcpy(inode_data.data,&HFS_FILE_FLAG,HFS_FLAG_SIZE);
    
    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(inode_data.data + HFS_FLAG_SIZE);
    initStat(fileStructure, mode);
    inode_value->file_structure = fileStructure;
    inode_value->has_external_data = false;
    inode_value->filename_len = filename.length();

    char* name_buf = inode_data.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());
    name_buf[inode_value->filename_len] = '\0';

    return inode_data;
}

void setBlobAndSize(rocksdb::DB* metaDataDB, HFS_KEY key,size_t writtenBytes){
    std::string data;
    hfs::db::getValue(metaDataDB,key,data);
    HFSInodeValueSerialized serialData;
    serialData.data = &data[0];
    serialData.size = data.size();
    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(serialData.data + HFS_FLAG_SIZE);
    inode_value->has_external_data = true;
    inode_value->file_structure.st_size = writtenBytes;
    hfs::db::rocksDBInsert(metaDataDB,key,serialData);
}

struct stat getFileStat(rocksdb::DB* metaDataDB, HFS_KEY key) {
    std::string valueData;

    rocksdb::Status status = metaDataDB->Get(rocksdb::ReadOptions(), std::to_string(key), &valueData);
    HFSInodeValueSerialized headerValue;
    headerValue.data = &valueData[0];
    headerValue.size = valueData.size();

    if (status.ok()) {
        uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

        if(*flag){
            HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + HFS_FLAG_SIZE);
            return metaData->file_structure;

        }else{
            HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
            return metaData->file_structure;
        }
    } else{
        std::cout << status.ToString() << std::endl;
    }
}
}

namespace db {

rocksdb::DB* createMetaDataDB(std::string metadir) {
    std::string db_path = hfs::path::getCurrentPath() + "/" + metadir;
    std::filesystem::remove_all(db_path);

    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);

    rocksdb::WriteOptions write_options;
    rocksdb::Slice start_key("");
    rocksdb::Slice end_key("\xFF");

    rocksdb::Status s = db->DeleteRange(write_options, db->DefaultColumnFamily(), start_key, end_key);
    if (!s.ok()) {
        std::cerr << "DeleteRange failed: " << s.ToString() << std::endl;
    } else {
        std::cout << "Database cleared successfully...\n";
    }

    return db;
}

rocksdb::DB* getDBPtr(struct fuse_context* context) {
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    return hfsState->getMetaDataDB();
}

rocksdb::Slice getKeySlice(HFS_KEY key) {
    return rocksdb::Slice(reinterpret_cast<const char*>(&key), sizeof(HFS_KEY));
}

rocksdb::Slice getValueSlice(const HFSInodeValueSerialized value) {
    return rocksdb::Slice(value.data, value.size);
}

int getValue(rocksdb::DB* db, HFS_KEY key, std::string& data) {
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), std::to_string(key), &data);
    if (!status.ok()) {
        return -1;
    }
    return 0;
}

std::string getFileNamefromKey(rocksdb::DB* db, HFS_KEY key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), std::to_string(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized headerValue;
        headerValue.data = &valueData[0];
        headerValue.size = valueData.size();

        uint8_t* flag = reinterpret_cast<uint8_t*>(headerValue.data);

        if(*flag){
            HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerValue.data + HFS_FLAG_SIZE);
            char* filenameBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE;
            char* filename = new char[metaData->filename_len + 1];
            memcpy(filename,filenameBuffer,metaData->filename_len);
            filename[metaData->filename_len] = '\0';
            std::string result(filename);
            delete[] filename;
            return result;
        }else{
            HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);
            char* filenameBuffer = headerValue.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE;
            char* filename = new char[metaData->filename_len + 1];
            memcpy(filename,filenameBuffer,metaData->filename_len);
            filename[metaData->filename_len] = '\0';
            std::string result(filename);
            delete[] filename;
            return result;
        }
    }
}

void* getMetaDatafromKey(rocksdb::DB* db, HFS_KEY key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        uint8_t* flag = reinterpret_cast<uint8_t*>(inodeData.data);

        if(*flag){
            HFSDirMetaData* dirMetaData = reinterpret_cast<HFSDirMetaData*>(inodeData.data + HFS_FLAG_SIZE);
            return dirMetaData;
        }else{
            HFSFileMetaData* fileMetaData = reinterpret_cast<HFSFileMetaData*>(inodeData.data + HFS_FLAG_SIZE);
            return fileMetaData;
        }
    }
}

void incrementParentDirLink(rocksdb::DB* db, HFS_KEY parentKey, HFS_KEY key){
    std::string valueData;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), std::to_string(parentKey), &valueData);

    HFSInodeValueSerialized currentHeaderField;
    currentHeaderField.data = &valueData[0];
    currentHeaderField.size = valueData.size();

    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(currentHeaderField.data + sizeof(HFS_DIR_FLAG));

    uint currentEntries = metaData->file_structure.st_nlink - 2; //Itself and "." entry not accounted for
    printf("Current entries in parent dir: %u\n",currentEntries);
    metaData->file_structure.st_nlink++;

    HFSInodeValueSerialized updatedHeaderField;
    updatedHeaderField.size = valueData.size() + sizeof(HFS_KEY);
    updatedHeaderField.data = new char[updatedHeaderField.size];
    memcpy(updatedHeaderField.data,currentHeaderField.data,valueData.size()); //Copy of existing data


    char* entriesPtr = updatedHeaderField.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1 + (currentEntries*sizeof(HFS_KEY));

    memcpy(entriesPtr,&key,sizeof(HFS_KEY)); //Insert new entry

    //rocksdb::Slice dbKey = getKeySlice(key);
    rocksdb::Slice dbValue = getValueSlice(updatedHeaderField);

    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(parentKey), dbValue);

    delete[] updatedHeaderField.data;
}

void deleteEntryAtParent(rocksdb::DB* db, HFS_KEY parentKey, HFS_KEY key){
    printf("entry\n");
    std::string valueData;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), std::to_string(parentKey), &valueData);

    HFSInodeValueSerialized headerData;
    headerData.data = &valueData[0];
    headerData.size = valueData.size();    
    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(headerData.data + HFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;

    std::vector<HFS_KEY> dirEntries;
    dirEntries.reserve(noOfEntries - 1);
    metaData->file_structure.st_nlink--;
    char* keyBuffer = headerData.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;

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
        keyBuffer = headerData.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
        memcpy(keyBuffer, dirEntries.data(),noOfEntries*sizeof(HFS_KEY));
    }

    headerData.size = valueData.size() - sizeof(HFS_KEY); 

    rocksdb::Slice dbValue = getValueSlice(headerData);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(parentKey), dbValue);
}

std::vector<HFS_KEY> getDirEntries(rocksdb::DB* db,HFS_KEY key){
    std::string valueData;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), std::to_string(key), &valueData);

    HFSInodeValueSerialized dirHeader;
    dirHeader.data = &valueData[0];
    dirHeader.size = valueData.size();

    HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(dirHeader.data + HFS_FLAG_SIZE);
    size_t noOfEntries = metaData->file_structure.st_nlink - 2;
    std::vector<HFS_KEY> dirEntries(noOfEntries);
    char* keyBuffer = dirHeader.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + metaData->filename_len + 1;
    HFS_KEY* entryKey = nullptr;
    for(int i = 0;i < noOfEntries;i++){
        entryKey = reinterpret_cast<HFS_KEY*>(keyBuffer);
        dirEntries[i] = *entryKey;
        std::cout << "entry: " << dirEntries[i] << "\n";
        keyBuffer += sizeof(HFS_KEY);
    }

    return dirEntries;

}

int rocksDBInsert(rocksdb::DB* db,HFS_KEY key,HFSInodeValueSerialized value){
    rocksdb::Slice valueSlice = hfs::db::getValueSlice(value);
    rocksdb::Status s = db->Put(rocksdb::WriteOptions(),std::to_string(key),valueSlice);
    if(!s.ok()){
        return -1;
    }
    return 0;
}

void updateMetaData(rocksdb::DB* db, HFS_KEY key, struct stat new_stat) {
    std::string valueData;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), std::to_string(key), &valueData);

    HFSInodeValueSerialized inodeData;
    inodeData.data = &valueData[0];
    inodeData.size = valueData.size();

    uint8_t* flag = reinterpret_cast<uint8_t*>(inodeData.data);

    if(!*flag){ //File
        HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(inodeData.data + sizeof(HFS_FILE_FLAG));
        metaData->file_structure = new_stat;
    } else{ //Directory
        HFSDirMetaData* metaData = reinterpret_cast<HFSDirMetaData*>(inodeData.data + sizeof(HFS_DIR_FLAG));
        metaData->file_structure = new_stat;
    }

    //rocksdb::Slice dbKey = getKeySlice(key);
    rocksdb::Slice dbValue = getValueSlice(inodeData);

    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), dbValue);
}

void getSerializedData(rocksdb::DB* db, HFS_KEY key,HFSInodeValueSerialized& keyValue){

    std::string data;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(),std::to_string(key),&data);

    #ifdef DEBUG
        if(!status.ok()){
            printf("Error retrieving value. Error: %s\n", status.ToString());
        }
    #endif

    keyValue.size = data.size();
    keyValue.data = new char[keyValue.size];
    memcpy(keyValue.data,&data[0],keyValue.size); 

}
}

namespace path {

std::string getCurrentPath() {
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return "";
    }
    return std::string(cwd);
}

std::string returnParentDir(const std::string& path) {
    if (path == "/") return "/";

    size_t lastSlash = path.find_last_of("/");
    if (lastSlash == std::string::npos) return ".";
    else if (lastSlash == 0) return "/";

    return path.substr(0, lastSlash);
}

std::string returnFilenameFromPath(const std::string& path) {
    if (path.empty()) return "";

    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    } else if (pos == path.length() - 1) {
        size_t end = pos;
        while (end > 0 && path[end - 1] == '/') {
            end--;
        }

        if (end == 0) {
            return "";
        }

        pos = path.find_last_of('/', end - 1);
        if (pos == std::string::npos) {
            return path.substr(0, end);
        } else {
            return path.substr(pos + 1, end - pos - 1);
        }
    } else {
        return path.substr(pos + 1);
    }
}

std::string getParentPath(const std::string& path) {
    if (path == "/") {
        return "/";
    }

    size_t pathLength = path.length();
    while (pathLength > 1 && path[pathLength - 1] == '/') {
        --pathLength;
    }

    size_t lastSlashPos = path.find_last_of('/', pathLength - 1);
    if (lastSlashPos == std::string::npos) {
        return ".";
    }

    if (lastSlashPos == 0) {
        return "/";
    }

    return path.substr(0, lastSlashPos);
}

int writeToLocalFile(const char *path, const char *buf, size_t size, off_t offset, HFS_KEY key,std::string datadir,rocksdb::DB* db){
    namespace fs = std::filesystem;
    std::string J = std::to_string(key / 100);
    std::string I = std::to_string(key);
    fs::path fullPath = fs::current_path() / datadir / J / I;

    if (!fs::exists(fullPath.parent_path())) {
        fs::create_directories(fullPath.parent_path());
    }

    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        return -1;
    }

    ssize_t bytesWritten = pwrite(fd, buf, size, offset);
    if (bytesWritten == -1) {
        close(fd);
        return -1;
    }

    close(fd);

    hfs::inode::setBlobAndSize(db,key,size);

    return bytesWritten;
}

int readFromLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir){
    namespace fs = std::filesystem;
    std::string J = std::to_string(key / 10000);
    std::string I = std::to_string(key);
    fs::path fullPath = fs::current_path() / datadir / J / I;

    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd == -1) {
        
        return -1;
    } 

    ssize_t bytesRead = pread(fd, (void*)buf, size, offset);
    if (bytesRead == -1) {
        close(fd);
        return -1;
    } 

    close(fd);

    return bytesRead;
}
} // namespace path
}// namespace hfs
