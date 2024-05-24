#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <cstring>

#define MIN_KEY 0
#define MAX_KEY UINT32_MAX

class KeyHandler {
private:
    uint32_t currentKey;
    std::unordered_map<const char*, uint64_t> map;
    std::queue<uint64_t> queue;

public:
    KeyHandler();

    int getNextKey();
    void makeNewEntry(uint32_t key, const char* path);
    void recycleKey(uint32_t key);
    uint32_t getKeyFromPath(const char* path);
    bool entryExists(const char* path);
};

#endif 
