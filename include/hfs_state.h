#ifndef HFS_STATE_H
#define HFS_STATE_H

#include <string>
#include <iostream>
#include "rocksdb/db.h"
#include "../include/hfs_KeyHandler.h"

#define DATA_THRESHOLD (4 * 1024);

class HFS_FileSystemState {
private:
    std::string mntdir;
    std::string metadir;
    std::string datadir;
    u_int dataThreshold;
    int maxInodeNum = 0;
    bool rootInitialized = false;
    HFS_KeyHandler* handler = nullptr;

public:
    // Constructor
    HFS_FileSystemState(std::string mntdir,
                        std::string metadir,
                        std::string datadir,
                        u_int dataThreshold,
                        HFS_KeyHandler* handler = nullptr);


    // Member functions
    void initalizeRoot();
    bool getRootInitFlag();
    bool getIDFlag();
    int getNextInodeNumber();
    void incrementInodeNumber();
    u_int getDataThreshold();
    void setKeyHandler(HFS_KeyHandler* newHandler);
    HFS_KeyHandler* getKeyHandler() const;
    std::string getDataDir();
    std::string getMetaDataDir();
};

#endif
