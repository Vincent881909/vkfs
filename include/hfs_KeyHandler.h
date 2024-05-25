#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <cstring>

#define MIN_KEY 0
#define MAX_KEY UINT32_MAX

typedef uint32_t HFS_KEY;

class HFS_KeyHandler {
private:
    HFS_KEY currentKey;
    std::unordered_map<std::string, uint64_t> map;
    std::queue<uint64_t> queue;

public:
    HFS_KeyHandler();

    int getNextKey();
    void makeNewEntry(HFS_KEY key, const char* path);
    void recycleKey(HFS_KEY key);
    int getKeyFromPath(const char* path,HFS_KEY &key);
    bool entryExists(const char* path);
    void eraseEntry(const char* path);
    int handleEntries(const char* path, HFS_KEY &key);
    int handleErase(const char* path);
};

#endif 
