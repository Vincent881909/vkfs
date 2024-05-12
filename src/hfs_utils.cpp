#include "../include/hfs_utils.h"
#include "../include/hash.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>

hfs_inode_key retrieveKey(const char* path){
    printf("Retrieve Key function called\n");
    struct hfs_inode_key key;
    if(strcmp(path,"/") == 0){  //Root directory
        key.inode_number = ROOT_INODE_ID;
        key.inode_number_hashed = murmur64(path, 0, 123);
    } else{
        printf("Retrieve Key for non-root calls not implemented yet...\n");
    }

    printf("Inode Number retrieved for path: %c, :%d",path,key.inode_number);
    printf("Inode Number Hashed retrieved for path: %c, :%ld",path,key.inode_number_hashed);

    return key;
}

struct stat retrieveStat(hfs_inode_key key){
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
    std::string valueData;
    rocksdb::Slice keySlice(reinterpret_cast<char*>(&key.inode_number_hashed), sizeof(key.inode_number_hashed));

    rocksdb::Status s = metaDataDB->Get(rocksdb::ReadOptions(), keySlice, &valueData);

    if (s.ok()) {
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(valueData.data());
        return inodeValue->file_structure;
    }else{printf("Error retrieving stat structure\n");}
}

void createMetaDataKey(char* path, int len, struct hfs_inode_key *inode, uint64_t maxInoderNumber) {
    // Print debug statements for all values
    printf("Path: %s\n", path);
    printf("Length: %d\n", len);

    // Set inode values
    inode->inode_number = maxInoderNumber;
    inode->inode_number_hashed = murmur64(path, len, 123);

    // Print debug statements for updated inode values
    printf("Inode Number: %d\n", inode->inode_number);
    printf("Inode Number Hashed: %ld\n", inode->inode_number_hashed);
}

void initStat(struct stat &statbuf,struct stat fileStructure){
    struct fuse_context* context = fuse_get_context();
    
    statbuf.st_mode = fileStructure.st_mode;
    statbuf.st_dev = fileStructure.st_dev;
    statbuf.st_gid = context->gid;
    statbuf.st_uid = context->uid;
    statbuf.st_size = 0;
    statbuf.st_blksize = 0;
    statbuf.st_blocks = 0;

    if S_ISREG(fileStructure.st_mode) {
        statbuf.st_nlink = 1; //Regular File
    } else {
        statbuf.st_nlink = 1; // Directory
    }
    
    time_t now = time(NULL);
    statbuf.st_atim.tv_nsec = 0;
    statbuf.st_mtim.tv_nsec = 0;
    statbuf.st_ctim.tv_sec = now;
    statbuf.st_ctim.tv_nsec = 0;

}

hfs_inode_value_serialized initInodeValue(struct stat fileStructure, std::string filename){
    struct hfs_inode_value_serialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    struct hfs_inode_value* inode_value = reinterpret_cast<struct hfs_inode_value*>(inode_data.data);
    initStat(inode_value->file_structure,fileStructure);
    inode_value->has_external_data = false;
    inode_value->filename_len = filename.length();

    char* name_buf = inode_data.data + HFS_INODE_VALUE_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());  // Correctly copy the filename
    name_buf[inode_value->filename_len] = '\0';  // Null terminate the filenam

    return inode_data;
}




