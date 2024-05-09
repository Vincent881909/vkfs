#include "hfs_state.h"

// Constructor definition
HFS_FileSystemState::HFS_FileSystemState(const std::string& mntdir,
                                         const std::string& metadir,
                                         const std::string& datadir,
                                         u_int dataThreshold,
                                         rocksdb::DB* metaDataDB,
                                         int maxInodeNum)
    : mntdir(mntdir), metadir(metadir), datadir(datadir),
      dataThreshold(dataThreshold), metaDataDB(metaDataDB),
      maxInodeNum(maxInodeNum) {}

// Definition of printState function
void HFS_FileSystemState::printState() const {
    std::cout << "Printing State...\n";
    std::cout << "Mount Directory: " << mntdir << std::endl;
    std::cout << "Metadata Directory: " << metadir << std::endl;
    std::cout << "Data Directory: " << datadir << std::endl;
    std::cout << "Data Threshold: " << dataThreshold << std::endl;
    std::cout << "Max Inode Number: " << maxInodeNum << std::endl;
}

// Definition of getMetaDataDB function
void* HFS_FileSystemState::getMetaDataDB() const {
    return metaDataDB;
}
