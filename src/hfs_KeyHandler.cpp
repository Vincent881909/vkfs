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
    map[path] = key;
}

void KeyHandler::recycleKey(uint32_t key) {
    queue.push(key);
}

uint32_t KeyHandler::getKeyFromPath(const char* path) {
    return map[path];
}

bool KeyHandler::entryExists(const char* path) {
    return map.find(path) != map.end();
}
