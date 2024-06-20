#ifndef VKFS_STATE_H
#define VKFS_STATE_H

#include <string>
#include <iostream>
#include "rocksdb/db.h"
#include "../include/vkfs_key_handler.h"

#define DATA_THRESHOLD (4 * 1024);

class VKFS_FileSystemState {
private:
    std::string mntdir;
    std::string metadir;
    std::string datadir;
    u_int dataThreshold;
    int maxInodeNum = 0;
    bool rootInitialized = false;
    VKFS_KeyHandler* handler = nullptr;

public:
    VKFS_FileSystemState(std::string mntdir,
                        std::string metadir,
                        std::string datadir,
                        u_int dataThreshold,
                        VKFS_KeyHandler* handler = nullptr);

    void initalizeRoot();
    bool getRootInitFlag();
    u_int getDataThreshold();
    void setKeyHandler(VKFS_KeyHandler* newHandler);
    VKFS_KeyHandler* getKeyHandler() const;
    std::string getDataDir();
    std::string getMetaDataDir();
};

#endif
