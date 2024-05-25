#include "../include/hfs_KeyHandler.h"
#include <shared_mutex>

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

int HFS_KeyHandler::getKeyFromPath(const char* path, HFS_KEY &key) {
    if (!entryExists(path)) {
        return -1;
    }

    key = map[std::string(path)];
    return 0;
}

void HFS_KeyHandler::eraseEntry(const char* path) {
    if (path == nullptr) {
        fprintf(stderr, "Invalid path (nullptr)\n");
        return;
    }

    std::string pathStr(path);

    if (!entryExists(path)) {
        fprintf(stderr, "Entry does not exist: %s\n", path);
        return;
    }
    map.erase(pathStr);
}

int HFS_KeyHandler::handleEntries(const char* path, HFS_KEY &key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (entryExists(path)) {
        return -1;
    }

    key = getNextKey();
    makeNewEntry(key, path);

    return 0;
}

int HFS_KeyHandler::handleErase(const char* path, HFS_KEY key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!entryExists(path)) {
        return -1;
    }

    printf("a\n");
    getKeyFromPath(path, key);
    printf("b\n");
    recycleKey(key);
    printf("c\n");
    printf("Path:%s\n", path);
    eraseEntry(path);
    printf("d\n");

    return 0;
}
