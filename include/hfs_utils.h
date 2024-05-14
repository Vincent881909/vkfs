#ifndef HFS_UTILS_H
#define HFS_UTILS_H

#include "hfs_inode.h"
#include <fuse.h>
#include <string>
#include <iostream>
#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <unistd.h>


static const char* ROOT_INODE = "/";
static const uint64_t ROOT_INODE_ID = 0;


void createMetaDataKey(const char* path, int len, struct hfs_inode_key &inode, uint64_t maxInoderNumber);

hfs_inode_value_serialized initInodeValue(struct stat fileStructure,std::string filename,mode_t mode);

void initStat(struct stat &statbuf,mode_t mode);

hfs_inode_key retrieveKey(const char* path);

struct stat retrieveStat(rocksdb::DB* metaDataDB,hfs_inode_key key);

rocksdb::DB* createMetaDataDB(std::string metadir);

std::string getCurrentPath();

std::string getParentDirectory(const std::string& path);

bool pathExists(const char* path,const char* parentPath);

std::string get_filename_from_path(const std::string& path);

rocksdb::Slice getKeySlice(const hfs_inode_key& key);

rocksdb::Slice getValueSlice(const hfs_inode_value_serialized value);

bool keyExists(hfs_inode_key key,rocksdb::DB* db);

void printValueForKey(rocksdb::DB* db, const hfs_inode_key& key);

void printInodeValue(hfs_inode_value &inodeValue);

void printStatStructure(const struct stat& statbuf);

struct stat returnStatFromKey(rocksdb::DB* db, const hfs_inode_key key);

void printMetaData(rocksdb::DB* db, const hfs_inode_key key);

struct hfs_inode_value returnInodeValuefromKey(rocksdb::DB* db, const hfs_inode_key key);

void updateMetaData(rocksdb::DB* db, struct hfs_inode_key key, std::string filename, struct hfs_inode_value inode_value,struct stat new_stat);

std::string getFileNamefromKey(rocksdb::DB* db,struct hfs_inode_key key);

#endif