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
#include <filesystem>

namespace fs = std::filesystem;

namespace hfs {
    static const char* ROOT_PATH = "/";
    static const HFS_KEY ROOT_KEY = 0;

    HFS_KeyHandler* getKeyHandler(struct fuse_context* context);

    namespace inode {
        HFSInodeValueSerialized initDirHeader(struct stat fileStructure, std::string filename,mode_t mode);
        HFSInodeValueSerialized initFileHeader(struct stat fileStructure, std::string filename, mode_t mode);
        void initStat(struct stat &statbuf, mode_t mode);
        void inlineWrite(HFSInodeValueSerialized& inodeData, const char* buf, size_t size, char*& newData);
        void truncateHeaderFile(HFS_KEY key,off_t len,size_t currentSize);
    }

    namespace path {
        std::string returnFilenameFromPath(const std::string& path);
        std::string getParentPath(const std::string& path);
        std::string getLocalFilePath(HFS_KEY key,std::string datadir);
        fs::path getExecutablePath();
        std::string getProjectRoot();
    }
}

namespace localFS {
        int writeToLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir);
        int readFromLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir);
        void migrateAndCleanData(HFS_KEY key,std::string datadir,HFSFileMetaData header,const char* path);
    }

#endif
