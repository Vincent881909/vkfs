#include <iostream>
#include "../include/hfs.h"
#include "../include/hfs_state.h"
#include "../include/hash.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>
#include <../include/hfs_inode.h>
#include <../include/hfs_utils.h>


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
    printf("Last access time: %status", ctime(&statbuf.st_atime));
    printf("Last modification time: %status", ctime(&statbuf.st_mtime));
    printf("Last status change time: %status", ctime(&statbuf.st_ctime));
}

void printHfsInodeValue(const hfs_inode_value& inodeValue) {
    std::cout << "Filename Length: " << inodeValue.filename_len << std::endl
              << "Has External Data: " << (inodeValue.has_external_data ? "Yes" : "No") << std::endl
              << "File Structure: " << std::endl;
    printStatInfo(inodeValue.file_structure);
}


void retrieveAndPrintRootNode(rocksdb::DB* db, struct hfs_inode_key *key) {
    printf("Inode Number: %d\n", key->inode_number);
    printf("Inode Number Hashed: %llu\n", key->inode_number_hashed);
    std::string valueData;
    rocksdb::Slice keySlice(reinterpret_cast<char*>(&key), sizeof(key));
    printf("Hashed key %llu\n", key->inode_number_hashed);

    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), keySlice, &valueData);

    if (status.ok()) {
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
        std::cerr << "Failed to retrieve the inode value or data mismatch: " << status.ToString() << std::endl;
    }
    exit(0);
}

int hfs_getattr(const char *path, struct stat *statbuf){
    printf("\n\n");
    printf("Get attribute called for %s\n",path);
    int res = 0;
    struct hfs_inode_key key = retrieveKey(path);
    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    memset(statbuf, 0, sizeof(struct stat));


    if(key.inode_number == ROOT_INODE_ID){ //Root directory
        printf("Is called for root directory\n");
        struct stat filestructure = retrieveStat(hfsState->getMetaDataDB(),key);

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        filestructure.st_atim = now;
        filestructure.st_mtim = now;

        filestructure.st_gid = context->gid;
        filestructure.st_uid = context->uid;

        *statbuf = filestructure;

        //Need to update metadata
        struct hfs_inode_value metadata = returnInodeValuefromKey(hfsState->getMetaDataDB(),key);
        updateMetaData(hfsState->getMetaDataDB(),key,"/",metadata,*statbuf);


        printStatStructure(filestructure);
        printf("\n\n\n");
    } else if(strncmp(getParentDirectory(path).c_str(), "/", 1) == 0){ //Parent direcotry is root
        printf("Is called for a file or dir in root directory\n");
        hfs_inode_key current_key;
        current_key.inode_number = ROOT_INODE_ID; //Inode ID of parent directory (root)

        std::string filenameStr = get_filename_from_path(path);
        const char* filename = filenameStr.c_str();

        current_key.inode_number_hashed = murmur64(path, strlen(filename), 123);

        printf("Key used for stat retrieval:\n");
        printf("Parent Inode: %d", current_key.inode_number);
        printf("Inode Hash: %llu", current_key.inode_number_hashed);
        printf("Filename: %s",filename);

        if(!keyExists(current_key,hfsState->getMetaDataDB())){
            printf("File does not exist!\n");
            return -ENOENT; //File does not exist
        }

        struct stat fstat = retrieveStat(hfsState->getMetaDataDB(),current_key);

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        fstat.st_atim = now;
        fstat.st_mtim = now;
        fstat.st_gid = context->gid;
        fstat.st_uid = context->uid;

        *statbuf = fstat;

        //Need to update metadata
        struct hfs_inode_value metadata = returnInodeValuefromKey(hfsState->getMetaDataDB(),current_key);
        updateMetaData(hfsState->getMetaDataDB(),key,filenameStr,metadata,*statbuf);
    }
    
    else {
        printf("GetAttr not yet fully implemented...\n");
        res = -2;
    }
    return res;
}

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    printf("\n\n");
    printf("Readdir called with path: %status",path);
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    if (filler(buf, ".", NULL, 0) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0) < 0) {
        return -ENOMEM;
    }



    return 0;
}

int hfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    printf("\n\n");

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    std::string parentDirStr = getParentDirectory(path);
    const char* parentDir = parentDirStr.c_str();
    std::string filenameStr = get_filename_from_path(path);
    const char* filename = filenameStr.c_str();

    printf("Create is called with path: %s \n",path);
    printf("Parent directory is %s \n",parentDir);
    printf("Filename is %s \n", filename);

    if(strcmp(parentDir,ROOT_INODE) == 0){ //Parent directory is root
        struct hfs_inode_key key;
        createMetaDataKey(path,strlen(filename),key,ROOT_INODE_ID);

        if(keyExists(key,hfsState->getMetaDataDB())){
            printf("File or directory with name: %s lready exists\n",filename);
            return -EEXIST;
        }
        else{printf("File or Direcory: %s does not yet exist. Creating file ..\n",filename);}
        
        struct stat statbuf;
        statbuf.st_ino = hfsState->getMaxInodeNumber();
        statbuf.st_mode = mode;
        hfsState->incrementInodeNumber();
        
        hfs_inode_value_serialized value = initInodeValue(statbuf,filenameStr,mode);
        rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
        rocksdb::Status status = metaDataDB->Put(rocksdb::WriteOptions(), getKeySlice(key), getValueSlice(value));

        if (!status.ok()) {
            fprintf(stderr, "Failed to insert %s into metadata DB: %s\n", filename,status.ToString().c_str());
        } else{
            printf("Succesfully inserted %s  into the metadata DB\n",filename);
        }


        if(keyExists(key,metaDataDB)){
            printf("Key found in DB after insertion...\n");
            printMetaData(metaDataDB,key);

        }else{
            printf("Key not found. It should be!\n");
        }
    }

    //Check filename does not exist already
    //Create file
    //Handle Permission Issues


    return 0;

}




void *hfs_init(struct fuse_conn_info *conn){
    printf("\n\n");
    printf("Initaliziing Hybrid File System...\n");

    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    
    if(!hfsState->getRootInitFlag()){ //Root directory not initliazed

        rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
        printf("Creating entry for root directory...\n");
        struct hfs_inode_key key;
        createMetaDataKey(ROOT_INODE,0,key,ROOT_INODE_ID);

        struct stat statbuf;
        lstat(ROOT_INODE, &statbuf);
        statbuf.st_ino = ROOT_INODE_ID;
        hfsState->incrementInodeNumber();

        hfs_inode_value_serialized value = initInodeValue(statbuf,ROOT_INODE,S_IFDIR | 0755);

        rocksdb::Slice dbKey = getKeySlice(key);
        rocksdb::Slice dbValue = getValueSlice(value);
        rocksdb::Status status = metaDataDB->Put(rocksdb::WriteOptions(), dbKey, dbValue);

        if (!status.ok()) {
            fprintf(stderr, "Failed to insert root inode into metadata DB: %s\n", status.ToString().c_str());
        } else{
            printf("Succesfully inserted root node into the metadata DB\n");
        }
    }

    return hfsState;

}

int hfs_utimens(const char *, const struct timespec tv[2]){
    //TODO
    return 0;
}

