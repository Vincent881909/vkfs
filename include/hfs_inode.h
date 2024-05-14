#ifndef HFS_INODE_H
#define HFS_INODE_H

#include <iostream>
#include <sys/stat.h>
#include <cstdint> 
#include <cstddef> 
#include <string>

struct hfs_inode_key{
    uint64_t inode_number;
    uint64_t inode_number_hashed;
};

struct hfs_inode_value{
    uint64_t filename_len;
    bool has_external_data;
    struct stat file_structure;
    
};

struct hfs_inode_value_serialized{
    uint64_t size;
    char* data;
};

static const size_t HFS_INODE_VALUE_SIZE = sizeof(hfs_inode_value);


#endif