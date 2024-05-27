#ifndef HFS_UTILS_H
#define HFS_UTILS_H

#include "hfs_inode.h"
#include "hfs_KeyHandler.h"
#include <fuse.h>
#include <string>
#include <iostream>
#include <vector>
#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <unistd.h>

namespace hfs {
    static const char* ROOT_PATH = "/";
    static const HFS_KEY ROOT_KEY = 0;

    namespace inode {
        HFSInodeValueSerialized initDirHeader(struct stat fileStructure, std::string filename,mode_t mode);
        HFSInodeValueSerialized initFileHeader(struct stat fileStructure, std::string filename, mode_t mode);
        HFSFileMetaData getFileHeader(rocksdb::DB* db,HFS_KEY key);
        void initStat(struct stat &statbuf, mode_t mode);
        struct stat getFileStat(rocksdb::DB* metaDataDB, HFS_KEY key);
        int getParentInodeNumber(const char* path);
        uint64_t getInodeFromPath(const char* path, rocksdb::DB* db, std::string filename);
        HFS_KeyHandler* getKeyHandler(struct fuse_context* context);
        int writeToKeyValue(rocksdb::DB* metaDataDB, HFS_KEY key,size_t size,const char* sourceBuf);
        void prepareWrite(HFSInodeValueSerialized& inodeData, const char* buf, size_t size, char*& newData);
        void setBlobAndSize(rocksdb::DB* metaDataDB, HFS_KEY key,size_t newSize);
        void truncateHeaderFile(rocksdb::DB* metaDataDB, HFS_KEY key,off_t len,size_t currentSize);
        void truncateLocalFile(rocksdb::DB* metaDataDB, HFS_KEY key,off_t len);
    }

    namespace db {
        rocksdb::DB* createMetaDataDB(std::string metadir);
        rocksdb::DB* getDBPtr(struct fuse_context* context);
        rocksdb::Slice getKeySlice(HFS_KEY key);
        rocksdb::Slice getValueSlice(const HFSInodeValueSerialized value);
        void* getMetaDatafromKey(rocksdb::DB* db, HFS_KEY key);
        void updateMetaData(rocksdb::DB* db, HFS_KEY key, struct stat new_stat);
        void incrementParentDirLink(rocksdb::DB* db, HFS_KEY parentKey, HFS_KEY key);
        void deleteEntryAtParent(rocksdb::DB* db, HFS_KEY parentKey, HFS_KEY key);
        std::string getFileNamefromKey(rocksdb::DB* db, HFS_KEY key);
        void getSerializedData(rocksdb::DB* db, HFS_KEY key,HFSInodeValueSerialized& KeyValue);
        std::vector<HFS_KEY> getDirEntries(rocksdb::DB* db,HFS_KEY key);
        int rocksDBInsert(rocksdb::DB* db,HFS_KEY key,HFSInodeValueSerialized value);
        int getValue(rocksdb::DB* db, HFS_KEY key, std::string& data);
    }

    namespace path {
        std::string getCurrentPath();
        std::string returnParentDir(const std::string& path);
        std::string returnFilenameFromPath(const std::string& path);
        std::string getParentPath(const std::string& path);
        int writeToLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir,rocksdb::DB* db);
        int readFromLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir);
        std::string getLocalFilePath(HFS_KEY key,std::string datadir);
        void migrateAndCleanData(rocksdb::DB* db,HFS_KEY key,std::string datadir,HFSFileMetaData header,const char* path);
    }

}

#endif
