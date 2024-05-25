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
        void initStat(struct stat &statbuf, mode_t mode);
        struct stat getFileStat(rocksdb::DB* metaDataDB, HFS_KEY key);
        int getParentInodeNumber(const char* path);
        uint64_t getInodeFromPath(const char* path, rocksdb::DB* db, std::string filename);
        HFS_KeyHandler* getKeyHandler(struct fuse_context* context);

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
        HFSInodeValueSerialized getSerializedData(rocksdb::DB* db, HFS_KEY key);
        std::vector<HFS_KEY> getDirEntries(rocksdb::DB* db,HFS_KEY key);
        void getReadBuffer(rocksdb::DB* db, HFS_KEY key, char*& buf, size_t size, size_t& bytesRead);
        void getWriteBuffer(rocksdb::DB* db, HFS_KEY key, char*& buf, size_t size, HFSInodeValueSerialized& inodeData);
        
    }

    namespace path {
        std::string getCurrentPath();
        std::string returnParentDir(const std::string& path);
        std::string returnFilenameFromPath(const std::string& path);
        std::string getParentPath(const std::string& path);
    }
}

#endif
