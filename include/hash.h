#ifndef HASH_H
#define HASH_H

#include <iostream>

//uint64_t generateHash(const char *path, int len, uint64_t seed);

uint64_t fnv1a(const char* data, size_t len);

// Function to generate hash for path and filename
uint64_t generateHash(const char* path, int filenameLength, uint64_t seed);

#endif