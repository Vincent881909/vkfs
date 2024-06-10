#ifndef HFS_INODE_H
#define HFS_INODE_H

#include <iostream>
#include <sys/stat.h>
#include <cstdint> 
#include <cstddef> 
#include <string>



struct HFSFileMetaData{
    size_t filename_len;
    bool has_external_data;
    struct stat file_structure; 
};

struct HFSDirMetaData{
    size_t filename_len;
    struct stat file_structure;
};

struct HFSInodeValueSerialized{
    uint16_t size;
    char* data;
};

static const size_t HFS_FILE_HEADER_SIZE = sizeof(HFSFileMetaData);
static const size_t HFS_DIR_HEADER_SIZE = sizeof(HFSDirMetaData);
static const size_t HFS_FLAG_SIZE = sizeof(uint8_t);
static const uint8_t HFS_FILE_FLAG = 0;
static const uint8_t HFS_DIR_FLAG = 1;
static const uint8_t HFS_MAX_FILE_LEN = 255;


#endif