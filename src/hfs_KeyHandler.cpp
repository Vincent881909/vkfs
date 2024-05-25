#include "../include/hfs_KeyHandler.h"

HFS_KeyHandler::HFS_KeyHandler() : currentKey(MIN_KEY) {}

int HFS_KeyHandler::getNextKey() {
    if (currentKey == MAX_KEY && queue.empty()) {
        return -1;  // overflow
    }
    HFS_KEY nextKey;
    if (!queue.empty()) {
        nextKey = queue.front();
        queue.pop();
    } else {
        nextKey = currentKey++;
    }
    return nextKey;
}

void HFS_KeyHandler::makeNewEntry(HFS_KEY key, const char* path) {
    map[std::string(path)] = key;
}

void HFS_KeyHandler::recycleKey(HFS_KEY key) {
    queue.push(key);
}

bool HFS_KeyHandler::entryExists(const char* path) {
    return map.find(std::string(path)) != map.end();
}

int HFS_KeyHandler::getKeyFromPath(const char* path,HFS_KEY &key) {
    if(!entryExists(path)){
        return -1;
    }

    key = map[std::string(path)];
    return 0;
}

void HFS_KeyHandler::eraseEntry(const char* path){
      map.erase(std::string(path));
    }

int HFS_KeyHandler::handleEntries(const char* path, HFS_KEY &key){
    if(entryExists(path)){
        return -1;
    }

    key = getNextKey();

    makeNewEntry(key, path);

    return 0;
}

int HFS_KeyHandler::handleErase(const char* path){
    if(!entryExists(path)){
    return -1;
    }

    HFS_KEY key;
    getKeyFromPath(path,key);
    recycleKey(key);
    eraseEntry(path);
    return 0;
}
