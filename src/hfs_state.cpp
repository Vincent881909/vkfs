#include "../include/hfs_state.h"

// Constructor definition
HFS_FileSystemState::HFS_FileSystemState(std::string mntdir,
                                         std::string metadir,
                                         std::string datadir,
                                         u_int dataThreshold,
                                         rocksdb::DB* metaDataDB,
                                         HFS_KeyHandler* handler)
    : mntdir(std::move(mntdir)), metadir(std::move(metadir)), datadir(std::move(datadir)),
      dataThreshold(dataThreshold), metaDataDB(metaDataDB), handler(handler) {}


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
rocksdb::DB* HFS_FileSystemState::getMetaDataDB() const {
    return metaDataDB;
}

void HFS_FileSystemState::initalizeRoot(){
    rootInitialized = true;
}

bool HFS_FileSystemState::getRootInitFlag() {
    return rootInitialized;
}

bool HFS_FileSystemState::getIDFlag() {
    return idsInitialized;
}

void HFS_FileSystemState::setIDFlag() {
    idsInitialized = true;
}

int HFS_FileSystemState::getNextInodeNumber(){
    return maxInodeNum;
}

void HFS_FileSystemState::incrementInodeNumber(){
    maxInodeNum++;
}

u_int HFS_FileSystemState::getDataThreshold(){
    return dataThreshold;
}

void HFS_FileSystemState::setKeyHandler(HFS_KeyHandler* newHandler) {
    handler = newHandler;
}

HFS_KeyHandler* HFS_FileSystemState::getKeyHandler() const {
    return handler;
}



