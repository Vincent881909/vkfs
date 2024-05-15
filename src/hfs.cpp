#include "../include/hfs.h"
#include "../include/hfs_state.h"
#include "../include/hash.h"
#include <../include/hfs_inode.h>
#include <../include/hfs_utils.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>
#include <vector>

int hfs_getattr(const char *path, struct stat *statbuf){
    printf("\n\n");
    char* path_copy = strdup(path);


    printf("Get attribute called for %s\n",path_copy);
    int res = 0;
    struct hfs_inode_key key = retrieveKey(path_copy);
    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    memset(statbuf, 0, sizeof(struct stat));


    if(strcmp(path,"/") == 0){ //Root directory
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

    } else if(strcmp(getParentDirectory(path_copy).c_str(), "/") == 0){ //Parent direcotry is root
        printf("Is called for a file or dir in root directory\n");
        hfs_inode_key current_key;
        current_key.inode_number = ROOT_INODE_ID; //Inode ID of parent directory (root)

        std::string filenameStr = get_filename_from_path(path_copy);
        const char* filename = filenameStr.c_str();

        current_key.inode_number_hashed = murmur64(path_copy, strlen(filename), 123);

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
        updateMetaData(hfsState->getMetaDataDB(),current_key,filenameStr,metadata,*statbuf);
    }
    
    else { //All other entries (deeper nested)
        printf("Else statement triggered...\n");
        hfs_inode_key nestedKey;
        nestedKey.inode_number = getParentInodeNumber(path);

        std::string filenameStr = get_filename_from_path(path_copy);
        const char* filename = filenameStr.c_str();

        nestedKey.inode_number_hashed = murmur64(path_copy, strlen(filename), 123);

        printf("Key used for stat retrieval:\n");
        printf("Parent Inode: %d", nestedKey.inode_number);
        printf("Inode Hash: %llu", nestedKey.inode_number_hashed);
        printf("Filename: %s",filename);

        if(!keyExists(nestedKey,hfsState->getMetaDataDB())){
            printf("File does not exist!\n");
            return -ENOENT; //File does not exist
        }

        struct stat fstat = retrieveStat(hfsState->getMetaDataDB(),nestedKey);

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        fstat.st_atim = now;
        fstat.st_mtim = now;
        fstat.st_gid = context->gid;
        fstat.st_uid = context->uid;

        *statbuf = fstat;

        //Need to update metadata
        struct hfs_inode_value metadata = returnInodeValuefromKey(hfsState->getMetaDataDB(),nestedKey);
        updateMetaData(hfsState->getMetaDataDB(),nestedKey,filenameStr,metadata,*statbuf);


        printf("GetAttr not yet fully implemented...\n");
        res = 0;
    }
    return res;
}


int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    printf("\n\n");
    char* path_copy = strdup(path);
    printf("Readdir called with path: %s\n",path_copy);
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    (void)filler;
    (void)buf;

    if (filler(buf, ".", NULL, 0) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0) < 0) {
        return -ENOMEM;
    }


    std::string currentPath(path_copy);
    std::string dirName = get_filename_from_path(currentPath);
    uint64_t dirInode = getInodeFromPath(path,hfsState->getMetaDataDB(),dirName);
   

    hfs_inode_key start_key = { dirInode, 0 };
    hfs_inode_key end_key = { dirInode + 1, 0 }; 

    rocksdb::ReadOptions options;
    std::unique_ptr<rocksdb::Iterator> it(hfsState->getMetaDataDB()->NewIterator(options));

    for (it->Seek(getKeySlice(start_key)); it->Valid() && it->key().compare(getKeySlice(end_key)) < 0; it->Next()) {

        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(const_cast<char*>(it->value().data()));
            // Deserialize the filename
        const char* filename = it->value().data() + HFS_INODE_VALUE_SIZE;
        printf("======Found file: %s\n", filename);


        rocksdb::Slice key = it->key();
        const hfs_inode_key* keyPtr = reinterpret_cast<const hfs_inode_key*>(key.data());
    
    
        printMetaData(hfsState->getMetaDataDB(),*keyPtr);
        std::string valueData;

        rocksdb::Status status = hfsState->getMetaDataDB()->Get(rocksdb::ReadOptions(), key, &valueData);

        if (status.ok()) {
            hfs_inode_value_serialized inodeData;
            inodeData.data = &valueData[0]; 
            inodeData.size = valueData.size();

            hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
         
            const char* filename = valueData.c_str() + sizeof(hfs_inode_value);
            printf("Found file: %s with filename len: %d\n", filename,inodeValue->filename_len);

            if (inodeValue->file_structure.st_ino == 0) {
                printf("Root entry found..\n");
            continue;
            }

            // Fill buf with metadata
            if (filler(buf, filename, &inodeValue->file_structure, 0) < 0) {
                return -ENOMEM;
            }


        } else if (status.IsNotFound()) {
            std::cerr << "Key not found in database." << std::endl;
        } else {
            std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
        }      
    }

    if (!it->status().ok()) {
        std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
        return -EIO;
    }


    return 0;
}

int hfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    printf("\n\n");
    char* path_copy = strdup(path);
    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    std::string parentDirStr = getParentDirectory(path_copy);
    const char* parentDir = parentDirStr.c_str();
    std::string filenameStr = get_filename_from_path(path_copy);
    const char* filename = filenameStr.c_str();

    printf("Create is called with path: %s \n",path_copy);
    printf("Parent directory is %s \n",parentDir);
    printf("Filename is %s \n", filename);

    struct hfs_inode_key key;
    uint64_t parentInode = getParentInodeNumber(path_copy);
    createMetaDataKey(path_copy,strlen(filename),key,parentInode);

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

    //Check filename does not exist already
    //Create file
    //Handle Permission Issues


    return 0;

}

int hfs_mkdir(const char *path, mode_t mode) {
    printf("\n\n");

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);

    std::string parentDirStr = getParentDirectory(path);
    const char* parentDir = parentDirStr.c_str();
    std::string dirnameStr = get_filename_from_path(path);
    const char* dirname = dirnameStr.c_str();

    printf("Mkdir is called with path: %s \n", path);
    printf("Parent directory is %s \n", parentDir);
    printf("Directory name is %s \n", dirname);

    struct hfs_inode_key key;
    uint64_t parentDirInode = getParentInodeNumber(path);
    createMetaDataKey(path, strlen(dirname), key, parentDirInode);

    if (keyExists(key, hfsState->getMetaDataDB())) {
        printf("Directory with name: %s already exists\n", dirname);
        return -EEXIST;
    } else {
        printf("Directory: %s does not yet exist. Creating directory...\n", dirname);
    }

    struct stat statbuf;
    statbuf.st_ino = hfsState->getMaxInodeNumber();
    statbuf.st_mode = mode | S_IFDIR;
    hfsState->incrementInodeNumber();

    // Initialize inode value
    hfs_inode_value_serialized value = initInodeValue(statbuf, dirnameStr, mode | S_IFDIR);
    rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
    rocksdb::Status status = metaDataDB->Put(rocksdb::WriteOptions(), getKeySlice(key), getValueSlice(value));

    if (!status.ok()) {
        fprintf(stderr, "Failed to insert %s into metadata DB: %s\n", dirname, status.ToString().c_str());
        return -EIO;
    } else {
        printf("Successfully inserted %s into the metadata DB\n", dirname);
    }

    if (keyExists(key, metaDataDB)) {
        printf("Key found in DB after insertion...\n");
        printMetaData(metaDataDB, key);
    } else {
        printf("Key not found. It should be!\n");
        return -EIO;
    }

    // Update parent's metadata
    hfs_inode_key parent_key = retrieveKey(parentDir);
    struct hfs_inode_value parent_metadata = returnInodeValuefromKey(metaDataDB, parent_key);
    parent_metadata.file_structure.st_nlink++; // Increment the link count for the parent directory
    updateMetaData(metaDataDB, parent_key, parentDirStr, parent_metadata, parent_metadata.file_structure);


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
