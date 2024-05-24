#include "../include/hfs_utils.h"
#include "../include/hash.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <filesystem>
#include <algorithm>

namespace hfs {
namespace debug {

void printMetaData(rocksdb::DB* db, const HFSInodeKey key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), db::getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        const char* filename = valueData.c_str() + sizeof(HFSFileMetaData);
    }
}

void printInodeValue(HFSFileMetaData &inodeValue) {
}

void printStatStructure(const struct stat& statbuf) {
    printf("Device ID: %ld\n", statbuf.st_dev);
    printf("Inode number: %ld\n", statbuf.st_ino);
    printf("File mode: %o\n", statbuf.st_mode);
    printf("Number of hard links: %ld\n", statbuf.st_nlink);
    printf("User ID: %d\n", statbuf.st_uid);
    printf("Group ID: %d\n", statbuf.st_gid);
    printf("Device ID (if special file): %ld\n", statbuf.st_rdev);
    printf("Total size, in bytes: %ld\n", statbuf.st_size);
    printf("Block size for filesystem I/O: %ld\n", statbuf.st_blksize);
    printf("Number of 512B blocks allocated: %ld\n", statbuf.st_blocks);
    printf("Time of last access: %ld\n", statbuf.st_atime);
    printf("Time of last modification: %ld\n", statbuf.st_mtime);
    printf("Time of last status change: %ld\n", statbuf.st_ctime);
}

void printValueForKey(rocksdb::DB* db, const HFSInodeKey& key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), db::getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        printInodeValue(*inodeValue);
    }
}

} // namespace debug

namespace inode {

KeyHandler* getKeyHandler(struct fuse_context* context){
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    return hfsState->getKeyHandler();

}

HFSInodeKey getKeyfromPath(const char* path) {
    struct HFSInodeKey key;
    key.inode_number = getParentInodeNumber(path);
    std::string filename = path::returnFilenameFromPath(path);
    const char* filenameCstr = filename.c_str();
    key.inode_number_hashed = generateHash(path, strlen(filenameCstr), 123);
    return key;
}

void setInodeKey(const char* path, int len, struct HFSInodeKey &inode, uint64_t maxInoderNumber) {

    inode.inode_number = maxInoderNumber;
    inode.inode_number_hashed = generateHash(path, len, 123);
}

void initStat(struct stat &statbuf, mode_t mode) {
    statbuf.st_mode = mode;

    if (S_ISREG(mode)) {
        statbuf.st_nlink = 1;
    } else if (S_ISDIR(mode)) {
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

HFSInodeValueSerialized initInodeValue(struct stat fileStructure, std::string filename, mode_t mode) {
    struct HFSInodeValueSerialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    /*
    struct HFSFileMetaData inode_val;
    initStat(fileStructure, mode);
    inode_val.file_structure = fileStructure;
    inode_val.has_external_data = false;
    inode_val.filename_len = filename.length();
    memcpy(inode_data.data,&inode_val,HFS_INODE_VALUE_SIZE);
    */

    
    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(inode_data.data);
    initStat(fileStructure, mode);
    inode_value->file_structure = fileStructure;
    inode_value->has_external_data = false;
    inode_value->filename_len = filename.length();
    

    printf("Filename length %zu\n",filename.length());

    char* name_buf = inode_data.data + HFS_INODE_VALUE_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());
    name_buf[inode_value->filename_len] = '\0';

    return inode_data;
}

struct stat getFileStat(rocksdb::DB* metaDataDB, HFSInodeKey key) {
    std::string valueData;

    rocksdb::Status status = metaDataDB->Get(rocksdb::ReadOptions(), db::getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(valueData.data());
        return inodeValue->file_structure;
    }
}

HFSInodeKey getKeyFromPath(const char* path, uint64_t parent_inode) {
    HFSInodeKey key;
    key.inode_number = parent_inode;
    std::string filenameStr = hfs::path::returnFilenameFromPath(path);
    const char* filename = filenameStr.c_str();
    key.inode_number_hashed = generateHash(path, strlen(filename), 123);

    return key;
}

int getParentInodeNumber(const char* path) {
    if (strcmp(path, "/") == 0) {
        return ROOT_INODE_ID;
    }

    std::string pathStr(path);
    std::string parentDirStr = path::returnParentDir(pathStr);

    if (parentDirStr == "/") {
        return ROOT_INODE_ID;
    }

    std::string parentPath = path::getParentPath(pathStr);

    std::vector<std::string> pathComponents;

    size_t pos = 0;
    while ((pos = parentPath.find('/')) != std::string::npos) {
        if (pos != 0) {
            pathComponents.push_back(parentPath.substr(0, pos));
        }
        parentPath.erase(0, pos + 1);
    }
    if (!parentPath.empty()) {
        pathComponents.push_back(parentPath);
    }

    uint64_t parent_inode = ROOT_INODE_ID;
    HFSInodeKey key;
    key.inode_number = parent_inode;

    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
    std::string currentPath = "/";

    for (const std::string& component : pathComponents) {
        currentPath = currentPath + component;
        key = getKeyFromPath(currentPath.c_str(), parent_inode);
        key.inode_number_hashed = generateHash(currentPath.c_str(), strlen(component.c_str()), 123);

        if (!db::keyExists(key, metaDataDB)) {
            return -1;
        }

        struct HFSFileMetaData inodeValue = db::getMetaDatafromKey(metaDataDB, key);
        parent_inode = inodeValue.file_structure.st_ino;
        currentPath = currentPath + "/";
    }

    return parent_inode;
}

uint64_t getInodeFromPath(const char* path, rocksdb::DB* db, std::string filename) {
    if (strcmp(path, ROOT_INODE) == 0) {
        return ROOT_INODE_ID;
    }

    uint64_t parentInode = getParentInodeNumber(path);

    HFSInodeKey start_key = { parentInode, 0 };
    HFSInodeKey end_key = { parentInode + 1, 0 };

    rocksdb::ReadOptions options;
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));

    for (it->Seek(db::getKeySlice(start_key)); it->Valid() && it->key().compare(db::getKeySlice(end_key)) < 0; it->Next()) {
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(const_cast<char*>(it->value().data()));
        const char* currentFilename = it->value().data() + HFS_INODE_VALUE_SIZE;

        if (strcmp(currentFilename, filename.c_str()) == 0) {
            return inodeValue->file_structure.st_ino;
        }
    }
}

}

namespace db {

rocksdb::DB* createMetaDataDB(std::string metadir) {
    std::string db_path = path::getCurrentPath() + "/" + metadir;
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

rocksdb::Slice getKeySlice(const HFSInodeKey& key) {
    return rocksdb::Slice(reinterpret_cast<const char*>(&key), sizeof(HFSInodeKey));
}

rocksdb::Slice getValueSlice(const HFSInodeValueSerialized value) {
    return rocksdb::Slice(value.data, value.size);
}

bool keyExists(HFSInodeKey key, rocksdb::DB* db) {
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &value);
    if (s.ok()) return true;
    else return false;
}

std::string getFileNamefromKey(rocksdb::DB* db, struct HFSInodeKey key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        debug::printStatStructure(inodeValue->file_structure);

        const char* filename = valueData.c_str() + sizeof(HFSFileMetaData);
        return filename;
    }
}

struct stat returnStatFromKey(rocksdb::DB* db, const HFSInodeKey key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        return inodeValue->file_structure;
    }
}

struct HFSFileMetaData getMetaDatafromKey(rocksdb::DB* db, const HFSInodeKey key) {
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        return *inodeValue;
    }
}

void updateMetaData(rocksdb::DB* db, struct HFSInodeKey key, struct stat new_stat) {
    std::string valueData;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    HFSInodeValueSerialized inodeData;
    inodeData.data = &valueData[0];
    inodeData.size = valueData.size();

    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
    inodeValue->file_structure = new_stat;


    rocksdb::Slice dbKey = getKeySlice(key);
    rocksdb::Slice dbValue = getValueSlice(inodeData);

    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), dbKey, dbValue);
}

HFSInodeValueSerialized getSerializedData(rocksdb::DB* db, struct HFSInodeKey key){

    HFSInodeValueSerialized value;
    std::string data;

    rocksdb::Status status = db->Get(rocksdb::ReadOptions(),hfs::db::getKeySlice(key),&data);

    #ifdef DEBUG
        if(!status.ok()){
            printf("Error retrieving value. Error: %s\n", status.ToString());
        }
    #endif

    value.data = &data[0]; 
    value.size = data.size();

    return value;

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

bool pathExists(const char* path, const char* parentPath) {
    struct HFSInodeKey key;
    key.inode_number_hashed = generateHash(parentPath, strlen(parentPath), 123);
    // TODO: Implement the logic
    return true;
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

size_t getNewFileSize(size_t currentSize,size_t toWrite,size_t offset){
    if(offset < currentSize){
        size_t potentialNewSize = offset + toWrite;
        return std::max(potentialNewSize,currentSize);
    }else{
        return offset + toWrite;
    }
}

} // namespace path
} // namespace hfs
