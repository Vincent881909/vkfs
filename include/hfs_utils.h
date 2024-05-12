#ifndef HFS_UTILS_H
#define HFS_UTILS_H

#include "hfs_inode.h"
#include <string>
#include <iostream>
#include "rocksdb/db.h"
#include <rocksdb/slice.h>


static const char* ROOT_INODE = "/";
static const uint64_t ROOT_INODE_ID = 0;


void createMetaDataKey(char* path, int len, struct hfs_inode_key *inode, uint64_t maxInoderNumber);

hfs_inode_value_serialized initInodeValue(struct stat fileStructure,std::string filename);

void initStat(struct stat &statbuf,struct stat fileStructure);

hfs_inode_key retrieveKey(const char* path);

struct stat retrieveStat(hfs_inode_key key);


#endif