#ifndef HFS_UTILS_H
#define HFS_UTILS_H

#include "hfs_inode.h"
#include <fuse.h>
#include <string>
#include <iostream>
#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <unistd.h>

namespace hfs {
    static const char* ROOT_INODE = "/";
    static const uint64_t ROOT_INODE_ID = 0;

    namespace inode {
        void setInodeKey(const char* path, int len, struct HFSInodeKey &inode, uint64_t maxInoderNumber);
        HFSInodeValueSerialized initInodeValue(struct stat fileStructure, std::string filename, mode_t mode);
        void initStat(struct stat &statbuf, mode_t mode);
        HFSInodeKey getKeyfromPath(const char* path);
        struct stat getFileStat(rocksdb::DB* metaDataDB, HFSInodeKey key);
        HFSInodeKey getKeyFromPath(const char* path, uint64_t parent_inode);
        int getParentInodeNumber(const char* path);
        uint64_t getInodeFromPath(const char* path, rocksdb::DB* db, std::string filename);
    }

    namespace db {
        rocksdb::DB* createMetaDataDB(std::string metadir);
        rocksdb::DB* getDBPtr(struct fuse_context* context);
        rocksdb::Slice getKeySlice(const HFSInodeKey& key);
        rocksdb::Slice getValueSlice(const HFSInodeValueSerialized value);
        bool keyExists(HFSInodeKey key, rocksdb::DB* db);
        void printValueForKey(rocksdb::DB* db, const HFSInodeKey& key);
        struct stat returnStatFromKey(rocksdb::DB* db, const HFSInodeKey key);
        struct HFSFileMetaData getMetaDatafromKey(rocksdb::DB* db, const HFSInodeKey key);
        void updateMetaData(rocksdb::DB* db, struct HFSInodeKey key, std::string filename, struct HFSFileMetaData inode_value, struct stat new_stat);
        std::string getFileNamefromKey(rocksdb::DB* db, struct HFSInodeKey key);
    }

    namespace path {
        std::string getCurrentPath();
        std::string returnParentDir(const std::string& path);
        bool pathExists(const char* path, const char* parentPath);
        std::string returnFilenameFromPath(const std::string& path);
        std::string getParentPath(const std::string& path);
    }

    namespace debug {
        void printInodeValue(HFSFileMetaData &inodeValue);
        void printStatStructure(const struct stat& statbuf);
        void printMetaData(rocksdb::DB* db, const HFSInodeKey key);
    }
}

#endif
