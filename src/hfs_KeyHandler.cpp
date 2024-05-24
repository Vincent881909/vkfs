#include "../include/hfs_KeyHandler.h"

KeyHandler::KeyHandler() : currentKey(MIN_KEY) {}

int KeyHandler::getNextKey() {
    if (currentKey == MAX_KEY && queue.empty()) {
        return -1;  // overflow
    }
    uint32_t nextKey;
    if (!queue.empty()) {
        nextKey = queue.front();
        queue.pop();
    } else {
        nextKey = currentKey++;
    }
    return nextKey;
}

void KeyHandler::makeNewEntry(uint32_t key, const char* path) {
    map[std::string(path)] = key;
}

void KeyHandler::recycleKey(uint32_t key) {
    queue.push(key);
}

uint32_t KeyHandler::getKeyFromPath(const char* path) {
    return map[std::string(path)];
}

bool KeyHandler::entryExists(const char* path) {
    return map.find(std::string(path)) != map.end();
}

void KeyHandler::eraseEntry(const char* path){
      map.erase(std::string(path));
    }

int KeyHandler::handleEntries(const char* path, uint32_t &key){
    if(entryExists(path)){
        return -1;
    }

    key = getNextKey();

    makeNewEntry(key, path);

    return 0;
}

int KeyHandler::handleErase(const char* path){
    if(!entryExists(path)){
    return -1;
    }

    uint32_t key = getKeyFromPath(path);
    recycleKey(key);
    eraseEntry(path);
    return 0;
}
