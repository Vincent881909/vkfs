#include "../include/vkfs_key_handler.h"

VKFS_KeyHandler::VKFS_KeyHandler() : currentKey(MIN_KEY) {}

int VKFS_KeyHandler::getNextKey() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (currentKey == MAX_KEY && queue.empty()) {
        return -1;  // overflow
    }
    VKFS_KEY nextKey;
    if (!queue.empty()) {
        nextKey = queue.front();
        queue.pop();
    } else {
        nextKey = currentKey++;
    }
    return nextKey;
}

void VKFS_KeyHandler::makeNewEntry(VKFS_KEY key, const char* path) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    map[std::string(path)] = key;
}

void VKFS_KeyHandler::recycleKey(VKFS_KEY key) {
    queue.push(key);
}

bool VKFS_KeyHandler::entryExists(const char* path) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return map.find(std::string(path)) != map.end();
}

int VKFS_KeyHandler::getKeyFromPath(const char* path, VKFS_KEY &key) {
    if (!entryExists(path)) {
        return -1;
    }
    std::shared_lock<std::shared_mutex> lock(mutex);
    key = map[std::string(path)];
    return 0;
}

void VKFS_KeyHandler::eraseEntry(const char* path) {
    if (path == nullptr) {
        return;
    }

    std::string pathStr(path);

    if (!entryExists(path)) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(mutex);
    map.erase(pathStr);
}

int VKFS_KeyHandler::handleEntries(const char* path, VKFS_KEY &key) {
    if (entryExists(path)) {
        return -1;
    }

    key = getNextKey();
    makeNewEntry(key, path);

    return 0;
}

int VKFS_KeyHandler::handleErase(const char* path, VKFS_KEY key) {
    if (!entryExists(path)) {
        return -1;
    }

    getKeyFromPath(path, key);
    recycleKey(key);
    eraseEntry(path);

    return 0;
}
