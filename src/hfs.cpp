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
    printf("Get attribute called for %s\n\n",path);

    struct HFSInodeKey key = getKeyfromPath(path);
    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = getDBPtr(context);
    struct stat fileStat;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);


    memset(statbuf, 0, sizeof(struct stat));


    if(strcmp(path,ROOT_INODE) == 0){ //Root directory

        fileStat = getFileStat(db,key);

        fileStat.st_atim = now; //Set Access time to now
        fileStat.st_mtim = now; //Set modified time to now

        fileStat.st_gid = context->gid; //Set group id
        fileStat.st_uid = context->uid; //Set user id

        *statbuf = fileStat;

        //Need to update metadata
        struct HFSFileMetaData currentMetaData = getMetaDatafromKey(db,key);
        std::string rootDir(ROOT_INODE);
        updateMetaData(db,key,rootDir,currentMetaData,*statbuf);


    } else if(strcmp(returnParentDir(path).c_str(), "/") == 0){ //Parent direcotry is root

        key.inode_number = ROOT_INODE_ID; //Inode ID of parent directory (root)
        std::string filenameStr = returnFilenameFromPath(path);
        const char* filename = filenameStr.c_str();
        key.inode_number_hashed = murmur64(path, strlen(filename), 123);

        if(!keyExists(key,db)){
            return -ENOENT; //File does not exist
        }

        fileStat = getFileStat(db,key);

        fileStat.st_atim = now; //Set Access time to now
        fileStat.st_mtim = now; //Set modified time to now

        fileStat.st_gid = context->gid; //Set group id
        fileStat.st_uid = context->uid; //Set user id

        *statbuf = fileStat;

        //Need to update metadata
        struct HFSFileMetaData metadata = getMetaDatafromKey(db,key);
        updateMetaData(db,key,filenameStr,metadata,*statbuf);
    }
    
    else { //All other entries

        key.inode_number = getParentInodeNumber(path);
        std::string filenameStr = returnFilenameFromPath(path);
        const char* filename = filenameStr.c_str();
        key.inode_number_hashed = murmur64(path, strlen(filename), 123);

        if(!keyExists(key,db)){
            return -ENOENT; //File does not exist
        }

        fileStat = getFileStat(db,key);

        fileStat.st_atim = now; //Set Access time to now
        fileStat.st_mtim = now; //Set modified time to now

        fileStat.st_gid = context->gid; //Set group id
        fileStat.st_uid = context->uid; //Set user id

        *statbuf = fileStat;

        //Need to update metadata
        struct HFSFileMetaData metadata = getMetaDatafromKey(db,key);
        updateMetaData(db,key,filenameStr,metadata,*statbuf);
    }

    return 0;
}


int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){

    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = getDBPtr(context);

    if (filler(buf, ".", NULL, 0) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0) < 0) {
        return -ENOMEM;
    }

    std::string currentPath(path);
    std::string dirName = returnFilenameFromPath(currentPath);
    uint64_t dirInode = getInodeFromPath(path,db,dirName);
   
    HFSInodeKey startKey = { dirInode, 0 };
    HFSInodeKey endKey = { dirInode + 1, 0 }; 

    rocksdb::ReadOptions options;
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));

    for (it->Seek(getKeySlice(startKey)); it->Valid() && it->key().compare(getKeySlice(endKey)) < 0; it->Next()) {

        //Deserialize the filename
        //const char* filename = it->value().data() + HFS_INODE_VALUE_SIZE;

        rocksdb::Slice key = it->key();
        std::string valueData;
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &valueData);


        if(!status.ok()){
            std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
        }

        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0]; 
        inodeData.size = valueData.size();
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        
        const char* filename = valueData.c_str() + HFS_INODE_VALUE_SIZE;

        if (inodeValue->file_structure.st_ino == 0) { //Do not list root entry
            continue;
        }

        // Fill buf with metadata
        if (filler(buf, filename, &inodeValue->file_structure, 0) < 0) {
            return -ENOMEM;
        }

    }

    if (!it->status().ok()) {
        std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
        return -EIO;
    }

    return 0;
}

int hfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    rocksdb::DB* db = getDBPtr(context);

    std::string parentDirStr = returnParentDir(path);
    const char* parentDir = parentDirStr.c_str();
    std::string filenameStr = returnFilenameFromPath(path);
    const char* filename = filenameStr.c_str();

    struct HFSInodeKey key;
    uint64_t parentInode = getParentInodeNumber(path);
    setInodeKey(path,strlen(filename),key,parentInode);

    if(keyExists(key,db)){
        printf("File or directory with name: %s lready exists\n",filename);
        return -EEXIST;
    }
    
    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = hfsState->getNextInodeNumber();
    statbuf.st_mode = mode;
    hfsState->incrementInodeNumber();
    
    HFSInodeValueSerialized value = initInodeValue(statbuf,filenameStr,mode);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), getKeySlice(key), getValueSlice(value));

    if (!status.ok()) {
        fprintf(stderr, "Failed to insert %s into metadata DB: %s\n", filename,status.ToString().c_str());
    } else{
        printf("Succesfully inserted %s  into the metadata DB\n",filename);
    }

    return 0;
}

int hfs_mkdir(const char *path, mode_t mode) {

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* db = getDBPtr(context);

    std::string parentDirStr = returnParentDir(path);
    const char* parentDir = parentDirStr.c_str();
    std::string dirnameStr = returnFilenameFromPath(path);
    const char* dirname = dirnameStr.c_str();

    struct HFSInodeKey key;
    uint64_t parentDirInode = getParentInodeNumber(path);
    setInodeKey(path, strlen(dirname), key, parentDirInode);

    if (keyExists(key, db)) {
        printf("Directory with name: %s already exists\n", dirname);
        return -EEXIST;
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = hfsState->getNextInodeNumber();
    statbuf.st_mode = mode | S_IFDIR;
    hfsState->incrementInodeNumber();

    // Initialize inode value
    HFSInodeValueSerialized value = initInodeValue(statbuf, dirnameStr, mode | S_IFDIR);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), getKeySlice(key), getValueSlice(value));

    if (!status.ok()) {
        fprintf(stderr, "Failed to insert %s into metadata DB: %s\n", dirname, status.ToString().c_str());
        return -EIO;
    } else {
        printf("Successfully inserted %s into the metadata DB\n", dirname);
    }

    // Update parent's metadata
    HFSInodeKey parentDirKey = getKeyfromPath(parentDir);
    struct HFSFileMetaData parentMetaData = getMetaDatafromKey(db, parentDirKey);
    parentMetaData.file_structure.st_nlink++; // Increment the link count for the parent directory
    updateMetaData(db, parentDirKey, parentDirStr, parentMetaData, parentMetaData.file_structure);

    return 0;
}

void *hfs_init(struct fuse_conn_info *conn){

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* db = getDBPtr(context);
    
    if(!hfsState->getRootInitFlag()){ //Root directory not initialized

        struct HFSInodeKey key;
        setInodeKey(ROOT_INODE,0,key,ROOT_INODE_ID);

        struct stat statbuf;
        memset(&statbuf, 0, sizeof(struct stat));
        lstat(ROOT_INODE, &statbuf);
        statbuf.st_ino = ROOT_INODE_ID;
        hfsState->incrementInodeNumber();

        HFSInodeValueSerialized value = initInodeValue(statbuf,ROOT_INODE,S_IFDIR | 0755);
        rocksdb::Slice dbKey = getKeySlice(key);
        rocksdb::Slice dbValue = getValueSlice(value);
        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), dbKey, dbValue);

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
