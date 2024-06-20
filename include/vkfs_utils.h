#ifndef VKFS_UTILS_H
#define VKFS_UTILS_H

#include "vkfs_inode.h"
#include "vkfs_key_handler.h"
#include "vkfs_inode.h"
#include "vkfs_rocksdb.h"
#include "vkfs_state.h"

#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <fuse.h>

#include <iostream>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace vkfs {
    static const char* ROOT_PATH = "/";
    static const VKFS_KEY ROOT_KEY = 0;

    VKFS_KeyHandler* getKeyHandler(struct fuse_context* context);

    namespace inode {
        VKFSHeaderSerialized initDirHeader(struct stat fileStructure, std::string filename,mode_t mode);
        VKFSHeaderSerialized initFileHeader(struct stat fileStructure, std::string filename, mode_t mode);
        void initStat(struct stat &statbuf, mode_t mode);
        void inlineWrite(VKFSHeaderSerialized& inodeData, const char* buf, size_t size, char*& newData);
        void truncateHeaderFile(VKFS_KEY key,off_t len,size_t currentSize);
    }

    namespace path {
        std::string returnFilenameFromPath(const std::string& path);
        std::string getParentPath(const std::string& path);
        std::string getLocalFilePath(VKFS_KEY key,std::string datadir);
        fs::path getExecutablePath();
        std::string getProjectRoot();
    }
}

namespace localFS {
        int writeToLocalFile(const char *path, const char *buf, size_t size, off_t offset,VKFS_KEY key,std::string datadir);
        int readFromLocalFile(const char *path, const char *buf, size_t size, off_t offset,VKFS_KEY key,std::string datadir);
        void migrateAndCleanData(VKFS_KEY key,std::string datadir,VKFSFileMetaData header,const char* path);
    }

#endif
