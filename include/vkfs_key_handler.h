#ifndef VKFS_KEYHANDLER_H
#define VKFS_KEYHANDLER_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <shared_mutex>

#define MIN_KEY 1
#define MAX_KEY UINT32_MAX

typedef uint32_t VKFS_KEY;

class VKFS_KeyHandler {
private:
    VKFS_KEY currentKey;
    std::unordered_map<std::string, VKFS_KEY> map;
    std::queue<VKFS_KEY> queue;
    mutable std::shared_mutex mutex;

public:
    VKFS_KeyHandler();

    int getNextKey();
    void makeNewEntry(VKFS_KEY key, const char* path);
    void recycleKey(VKFS_KEY key);
    int getKeyFromPath(const char* path,VKFS_KEY &key);
    bool entryExists(const char* path);
    void eraseEntry(const char* path);
    int handleEntries(const char* path, VKFS_KEY &key);
    int handleErase(const char* path,VKFS_KEY key);
};

#endif 
