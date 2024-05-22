/*
Adpated from original source code of TableFS
*/

#include <iostream>
#include <cstring>

uint64_t fnv1a(const char* data, size_t len) {
    const uint64_t fnv_prime = 1099511628211ULL;
    const uint64_t offset_basis = 14695981039346656037ULL;

    uint64_t hash = offset_basis;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= fnv_prime;
    }
    return hash;
}


// Function to generate hash for path and filename
uint64_t generateHash(const char* path, int filenameLength, uint64_t seed) {
    // Calculate hash for path
    uint64_t pathHash = fnv1a(path, strlen(path));

    // Combine path hash with filename length
    uint64_t combinedHash = pathHash ^ (filenameLength * seed);

    return combinedHash;
}



