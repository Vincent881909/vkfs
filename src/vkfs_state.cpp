#include "../include/vkfs_state.h"

// Constructor definition
VKFS_FileSystemState::VKFS_FileSystemState(std::string mntdir,
                                         std::string metadir,
                                         std::string datadir,
                                         u_int dataThreshold,
                                         VKFS_KeyHandler* handler)
    : mntdir(std::move(mntdir)), metadir(std::move(metadir)), datadir(std::move(datadir)),
      dataThreshold(dataThreshold), handler(handler) {}

void VKFS_FileSystemState::initalizeRoot(){
    rootInitialized = true;
}

bool VKFS_FileSystemState::getRootInitFlag() {
    return rootInitialized;
}

int VKFS_FileSystemState::getNextInodeNumber(){
    return maxInodeNum;
}

void VKFS_FileSystemState::incrementInodeNumber(){
    maxInodeNum++;
}

u_int VKFS_FileSystemState::getDataThreshold(){
    return dataThreshold;
}

void VKFS_FileSystemState::setKeyHandler(VKFS_KeyHandler* newHandler) {
    handler = newHandler;
}

VKFS_KeyHandler* VKFS_FileSystemState::getKeyHandler() const {
    return handler;
}

std::string VKFS_FileSystemState::getDataDir(){
    return datadir;
}

std::string VKFS_FileSystemState::getMetaDataDir(){
    return metadir;
}



