#include <iostream>
#include "../include/hfs.h"
#include "../include/hfs_state.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>
#include <../include/hfs_inode.h>
#include <../include/hfs_utils.h>

int hfs_getattr(const char *path, struct stat *statbuf){
    int res = 0;
    struct hfs_inode_key key = retrieveKey(path);
    struct fuse_context* context = fuse_get_context();

    memset(statbuf, 0, sizeof(struct stat));

    if(key.inode_number == ROOT_INODE_ID){ //Root directory
        struct stat filestructure = retrieveStat(key);
        struct timespec now;
        now.tv_sec = time(NULL);  // Get current time in seconds
        now.tv_nsec = 0;          // No additional nanoseconds
        filestructure.st_atim = now;  
        filestructure.st_mtim = now;
        filestructure.st_gid = context->gid;
        filestructure.st_uid = context->uid;
        filestructure.st_mode = S_IFDIR | 0755;
        filestructure.st_nlink = 2;

        *statbuf = filestructure;
    } else {
        printf("GetAtte not yet fully implemented...\n");
        res = -2;
    }
    return res;
}

int hfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *){
    return 0;
}




void printStatInfo(const struct stat& statbuf) {
    printf("File type: ");
    if (S_ISREG(statbuf.st_mode)) printf("regular file\n");
    else if (S_ISDIR(statbuf.st_mode)) printf("directory\n");
    else if (S_ISCHR(statbuf.st_mode)) printf("character device\n");
    else if (S_ISBLK(statbuf.st_mode)) printf("block device\n");
    else if (S_ISFIFO(statbuf.st_mode)) printf("FIFO/pipe\n");
    else if (S_ISLNK(statbuf.st_mode)) printf("symlink\n");
    else if (S_ISSOCK(statbuf.st_mode)) printf("socket\n");
    else printf("unknown\n");

    printf("File size: %lld bytes\n", (long long) statbuf.st_size);
    printf("Number of hard links: %ld\n", (long) statbuf.st_nlink);
    printf("Owner UID: %d\n", statbuf.st_uid);
    printf("Group GID: %d\n", statbuf.st_gid);
    printf("Device ID (if special file): %ld\n", (long) statbuf.st_rdev);
    printf("Inode number: %ld\n", (long) statbuf.st_ino); // Adding st_ino
    printf("Last access time: %s", ctime(&statbuf.st_atime));
    printf("Last modification time: %s", ctime(&statbuf.st_mtime));
    printf("Last status change time: %s", ctime(&statbuf.st_ctime));
}

void printHfsInodeValue(const hfs_inode_value& inodeValue) {
    std::cout << "Filename Length: " << inodeValue.filename_len << std::endl
              << "Has External Data: " << (inodeValue.has_external_data ? "Yes" : "No") << std::endl
              << "File Structure: " << std::endl;
    printStatInfo(inodeValue.file_structure);
}

void retrieveAndPrintRootNode(rocksdb::DB* db, struct hfs_inode_key *key) {
    printf("Inode Number: %d\n", key->inode_number);
    printf("Inode Number Hashed: %ld\n", key->inode_number_hashed);
    std::string valueData;
    rocksdb::Slice keySlice(reinterpret_cast<char*>(&key->inode_number_hashed), sizeof(key->inode_number_hashed));
    printf("Hashed key %ld\n", key->inode_number_hashed);

    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), keySlice, &valueData);

    if (s.ok()) {
        // Assuming the data directly matches the size of hfs_inode_value
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(valueData.data());
        printf("========\n");
        printHfsInodeValue(*inodeValue);

        // Print the values
        std::cout << "Filename Length: " << inodeValue->filename_len << std::endl;
        std::cout << "Has External Data: " << (inodeValue->has_external_data ? "Yes" : "No") << std::endl;
        printStatInfo(inodeValue->file_structure);
        // You can add more fields to print from struct stat as needed
    } else {
        std::cerr << "Failed to retrieve the inode value or data mismatch: " << s.ToString() << std::endl;
    }
    exit(0);
}


void *hfs_init(struct fuse_conn_info *conn){
    printf("Initaliziing Hybrid File System...\n");
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    hfsState->printState();
    if(!hfsState->getRootInitFlag()){ //Root directory not initliazed
        rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
        printf("Creating entry for root directory...\n");
        struct hfs_inode_key key;
        createMetaDataKey(NULL,0,&key,hfsState->getMaxInodeNumber());
        struct stat statbuf;
        lstat(ROOT_INODE, &statbuf);
        statbuf.st_ino = hfsState->getMaxInodeNumber();
        printStatInfo(statbuf);
        hfsState->incrementInodeNumber();
        hfs_inode_value_serialized value = initInodeValue(statbuf,ROOT_INODE);

        hfs_inode_value* inode_value = reinterpret_cast<hfs_inode_value*>(value.data);

        rocksdb::Slice keySlice(reinterpret_cast<char*>(&key.inode_number_hashed), sizeof(key.inode_number_hashed));
        rocksdb::Slice valueSlice(value.data, value.size);
        rocksdb::Status s = metaDataDB->Put(rocksdb::WriteOptions(), keySlice, valueSlice);

        if (!s.ok()) {
            fprintf(stderr, "Failed to insert root inode into metadata DB: %s\n", s.ToString().c_str());
        } else{
            printf("Succesfully inserted root node into the metadata DB\n");
        }


    }

}