#ifndef VKFS_INODE_H
#define VKFS_INODE_H

#include <iostream>
#include <sys/stat.h>
#include <cstdint> 
#include <cstddef> 
#include <string>

struct VKFSFileMetaData{
    size_t filename_len;
    bool has_external_data;
    struct stat file_structure; 
};

struct VKFSDirMetaData{
    size_t filename_len;
    struct stat file_structure;
};

struct VKFSHeaderSerialized{
    uint16_t size;
    char* data;
};

static const size_t VKFS_FILE_HEADER_SIZE = sizeof(VKFSFileMetaData);
static const size_t VKFS_DIR_HEADER_SIZE = sizeof(VKFSDirMetaData);
static const size_t VKFS_FLAG_SIZE = sizeof(uint8_t);
static const uint8_t VKFS_FILE_FLAG = 0;
static const uint8_t VKFS_DIR_FLAG = 1;
static const uint8_t VKFS_MAX_FILE_LEN = 255;


#endif