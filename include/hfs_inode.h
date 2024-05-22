#ifndef HFS_INODE_H
#define HFS_INODE_H

#include <iostream>
#include <sys/stat.h>
#include <cstdint> 
#include <cstddef> 
#include <string>

struct HFSInodeKey{
    uint64_t inode_number;
    uint64_t inode_number_hashed;
};

struct HFSFileMetaData{
    size_t filename_len;
    bool has_external_data;
    struct stat file_structure; 
};

struct HFSInodeValueSerialized{
    uint64_t size;
    char* data;
};

static const size_t HFS_INODE_VALUE_SIZE = sizeof(HFSFileMetaData);


#endif