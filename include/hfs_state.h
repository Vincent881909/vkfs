#ifndef HFS_STATE_H
#define HFS_STATE_H

#include <string>
#include <iostream>
#include "rocksdb/db.h"

class HFS_FileSystemState {
private:
    std::string mntdir;
    std::string metadir;
    std::string datadir;
    u_int dataThreshold;
    int maxInodeNum;
    rocksdb::DB* metaDataDB;

public:
    // Constructor
    HFS_FileSystemState(const std::string& mntdir,
                        const std::string& metadir,
                        const std::string& datadir,
                        u_int dataThreshold,
                        rocksdb::DB* metaDataDB,
                        int maxInodeNum = 0);

    // Member functions
    void printState() const;
    void* getMetaDataDB() const;
};

#endif
