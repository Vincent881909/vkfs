#include "../include/hfs_state.h"

// Constructor definition
HFS_FileSystemState::HFS_FileSystemState(std::string mntdir,
                                         std::string metadir,
                                         std::string datadir,
                                         u_int dataThreshold,
                                         HFS_KeyHandler* handler)
    : mntdir(std::move(mntdir)), metadir(std::move(metadir)), datadir(std::move(datadir)),
      dataThreshold(dataThreshold), handler(handler) {}

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

std::string HFS_FileSystemState::getDataDir(){
    return datadir;
}

std::string HFS_FileSystemState::getMetaDataDir(){
    return metadir;
}



