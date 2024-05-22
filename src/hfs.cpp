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

int hfs_getattr(const char *path, struct stat *statbuf) {

    #ifdef DEBUG
        printf("getattr called with path: %s\n", path);
    #endif

    struct HFSInodeKey key = hfs::inode::getKeyfromPath(path);
    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);
    struct stat fileStat;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    memset(statbuf, 0, sizeof(struct stat));

    if (strcmp(path, hfs::ROOT_INODE) == 0) { // Root directory
        fileStat = hfs::inode::getFileStat(db, key);

        fileStat.st_atim = now; // Set Access time to now
        fileStat.st_mtim = now; // Set modified time to now

        fileStat.st_gid = context->gid; // Set group id
        fileStat.st_uid = context->uid; // Set user id

        *statbuf = fileStat;

        // Need to update metadata
        struct HFSFileMetaData currentMetaData = hfs::db::getMetaDatafromKey(db, key);
        std::string rootDir(hfs::ROOT_INODE);
        hfs::db::updateMetaData(db, key,*statbuf);
    }   
    else if (strcmp(hfs::path::returnParentDir(path).c_str(), "/") == 0) { // Parent directory is root

        key.inode_number = hfs::ROOT_INODE_ID; // Inode ID of parent directory (root)
        std::string filenameStr = hfs::path::returnFilenameFromPath(path);
        const char* filename = filenameStr.c_str();
        key.inode_number_hashed = generateHash(path, strlen(filename), 123);

        if (!hfs::db::keyExists(key, db)) {
            #ifdef DEBUG
                printf("Returning -ENONENT\n\n");
            #endif
            return -ENOENT; // File does not exist
        }

        fileStat = hfs::inode::getFileStat(db, key);

        fileStat.st_atim = now; // Set Access time to now
        fileStat.st_mtim = now; // Set modified time to now

        fileStat.st_gid = context->gid; // Set group id
        fileStat.st_uid = context->uid; // Set user id

        *statbuf = fileStat;

        // Need to update metadata
        struct HFSFileMetaData metadata = hfs::db::getMetaDatafromKey(db, key);
        hfs::db::updateMetaData(db, key,*statbuf);

    } else { // All other entries

        key.inode_number = hfs::inode::getParentInodeNumber(path);
        std::string filenameStr = hfs::path::returnFilenameFromPath(path);
        const char* filename = filenameStr.c_str();
        key.inode_number_hashed = generateHash(path, strlen(filename), 123);

        if (!hfs::db::keyExists(key, db)) {
            #ifdef DEBUG
                printf("Returning -ENONENT\n\n");
            #endif
            return -ENOENT; // File does not exist
        }

        fileStat = hfs::inode::getFileStat(db, key);

        fileStat.st_atim = now; // Set Access time to now
        fileStat.st_mtim = now; // Set modified time to now

        fileStat.st_gid = context->gid; // Set group id
        fileStat.st_uid = context->uid; // Set user id

        *statbuf = fileStat;

        // Need to update metadata
        struct HFSFileMetaData metadata = hfs::db::getMetaDatafromKey(db, key);
        hfs::db::updateMetaData(db, key,*statbuf);
    }

    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return 0;
}

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

    #ifdef DEBUG
        printf("readdir called with path: %s\n", path);
    #endif

    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    if (filler(buf, ".", NULL, 0) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0) < 0) {
        return -ENOMEM;
    }

    std::string currentPath(path);
    std::string dirName = hfs::path::returnFilenameFromPath(currentPath);
    uint64_t dirInode = hfs::inode::getInodeFromPath(path, db, dirName);
   
    HFSInodeKey startKey = { dirInode, 0 };
    HFSInodeKey endKey = { dirInode + 1, 0 };

    rocksdb::ReadOptions options;
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));

    for (it->Seek(hfs::db::getKeySlice(startKey)); it->Valid() && it->key().compare(hfs::db::getKeySlice(endKey)) < 0; it->Next()) {
        rocksdb::Slice key = it->key();
        std::string valueData;
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &valueData);

        #ifdef DEBUG
            printf("\n\nentry found\n");
        #endif

        if (!status.ok()) {
            #ifdef DEBUG
                printf("Failed to retrieve key\n");
            #endif  
        }

        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];
        inodeData.size = valueData.size();
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);

        const char* filename = valueData.c_str() + HFS_INODE_VALUE_SIZE;
        
        #ifdef DEBUG
            printf("Filename: %s\n",filename);
        #endif

        if (inodeValue->file_structure.st_ino == 0) { // Do not list root entry
            continue;
        }

        

        #ifdef DEBUG
            hfs::debug::printStatStructure(inodeValue->file_structure);
        #endif

        // Fill buf with metadata
        if (filler(buf, filename, &inodeValue->file_structure, 0) < 0) {
        #ifdef DEBUG
            printf("Returning -ENOMEM\n\n");
        #endif  
            return -ENOMEM;
        }
    }

    if (!it->status().ok()) {
        #ifdef DEBUG
            printf("Iterator error\n");
            printf("Returning -EIO\n\n");
        #endif        
        return -EIO;
    }

    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return 0;
}

int hfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    #ifdef DEBUG
        printf("create called with path: %s and mode %u\n",path,mode);
    #endif


    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    std::string parentDirStr = hfs::path::returnParentDir(path);
    const char* parentDir = parentDirStr.c_str();
    std::string filenameStr = hfs::path::returnFilenameFromPath(path);
    const char* filename = filenameStr.c_str();

    struct HFSInodeKey key;
    uint64_t parentInode = hfs::inode::getParentInodeNumber(path);
    hfs::inode::setInodeKey(path, strlen(filename), key, parentInode);

    if (hfs::db::keyExists(key, db)) {
        #ifdef DEBUG
            printf("Returning -EEXIST\n\n");
        #endif
        return -EEXIST;
    }

    //Store parent inode number for subsequent read or write calls to avoid frequent  lookup
    fi->fh = key.inode_number;

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = hfsState->getNextInodeNumber();
    statbuf.st_mode = S_IFREG | mode;
    hfsState->incrementInodeNumber();

    HFSInodeValueSerialized value = hfs::inode::initInodeValue(statbuf, filenameStr, mode);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), hfs::db::getKeySlice(key), hfs::db::getValueSlice(value));

    #ifdef DEBUG
        if (!status.ok()) {
            printf("Failed to insert %s into metadata DB: %s\n", filename, status.ToString().c_str());
        }
        printf("Returning 0\n\n");
    #endif

    return 0;
}

int hfs_mkdir(const char *path, mode_t mode) {
    #ifdef DEBUG
        printf("mkdir called with path: %s and mode %u\n",path,mode);
    #endif

    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    std::string parentDirStr = hfs::path::returnParentDir(path);
    const char* parentDir = parentDirStr.c_str();
    std::string dirnameStr = hfs::path::returnFilenameFromPath(path);
    const char* dirname = dirnameStr.c_str();

    struct HFSInodeKey key;
    uint64_t parentDirInode = hfs::inode::getParentInodeNumber(path);
    hfs::inode::setInodeKey(path, strlen(dirname), key, parentDirInode);

    if (hfs::db::keyExists(key, db)) {
        #ifdef DEBUG
            printf("Returning -EXISTS\n\n");
        #endif
        return -EEXIST;
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = hfsState->getNextInodeNumber();
    statbuf.st_mode = mode | S_IFDIR;
    hfsState->incrementInodeNumber();

    HFSInodeValueSerialized value = hfs::inode::initInodeValue(statbuf, dirnameStr, mode | S_IFDIR);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), hfs::db::getKeySlice(key), hfs::db::getValueSlice(value));


    #ifdef DEBUG
        if (!status.ok()) {
            printf("Failed to insert %s into metadata DB: %s\n", dirname, status.ToString().c_str());
            printf("Returning -EIO\n\n");
            return -EIO;
        } 
    #endif

    // Update parent's metadata
    HFSInodeKey parentDirKey = hfs::inode::getKeyfromPath(parentDir);
    struct HFSFileMetaData parentMetaData = hfs::db::getMetaDatafromKey(db, parentDirKey);
    parentMetaData.file_structure.st_nlink++; // Increment the link count for the parent directory
    hfs::db::updateMetaData(db, parentDirKey, parentMetaData.file_structure);
    
    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return 0;
}

void *hfs_init(struct fuse_conn_info *conn) {
    #ifdef DEBUG
        printf("init called\n");
    #endif
    struct fuse_context* context = fuse_get_context();
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    if (!hfsState->getRootInitFlag()) { // Root directory not initialized
        struct HFSInodeKey key;
        hfs::inode::setInodeKey(hfs::ROOT_INODE, 0, key, hfs::ROOT_INODE_ID);

        struct stat statbuf;
        memset(&statbuf, 0, sizeof(struct stat));
        lstat(hfs::ROOT_INODE, &statbuf);
        statbuf.st_ino = hfs::ROOT_INODE_ID;
        hfsState->incrementInodeNumber();

        HFSInodeValueSerialized value = hfs::inode::initInodeValue(statbuf, hfs::ROOT_INODE, S_IFDIR | 0755);
        rocksdb::Slice dbKey = hfs::db::getKeySlice(key);
        rocksdb::Slice dbValue = hfs::db::getValueSlice(value);
        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), dbKey, dbValue);

        #ifdef DEBUG
            if (!status.ok()) {
                printf("Failed to insert root inode into metadata DB: %s\n", status.ToString().c_str());
            }
        #endif
    }

    //Need to free heap memory

    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return hfsState;
}

int hfs_utimens(const char *path, const struct timespec tv[2]) {

    #ifdef DEBUG
        printf("utimens called with path %s\n", path);
    #endif

    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    HFSInodeKey key = hfs::inode::getKeyfromPath(path);

    if (!hfs::db::keyExists(key, db)) {
        #ifdef DEBUG
            printf("Returning -ENOENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    struct HFSFileMetaData metadata = hfs::db::getMetaDatafromKey(db, key);
    metadata.file_structure.st_atim = tv[0];
    metadata.file_structure.st_mtim = tv[1];
    hfs::db::updateMetaData(db, key, metadata.file_structure);

    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return 0;
}


int hfs_unlink(const char *path){

    #ifdef DEBUG
        printf("unlink called with path %s\n", path);
    #endif

    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    HFSInodeKey key = hfs::inode::getKeyfromPath(path);

    if (!hfs::db::keyExists(key, db)) {
        #ifdef DEBUG
            printf("Returning -ENOENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), hfs::db::getKeySlice(key));

    #ifdef DEBUG
        if (!status.ok()) {
            printf("Error during deletion\n"); 
        }

        printf("Returning 0\n\n");
    #endif

    return 0;

}

int hfs_rmdir(const char *path){


    #ifdef DEBUG
        printf("rmdir called with path %s\n",path);
    #endif

    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    HFSInodeKey key = hfs::inode::getKeyfromPath(path);

    if (!hfs::db::keyExists(key, db)) {
        #ifdef DEBUG
            printf("Returning -ENOENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), hfs::db::getKeySlice(key));
    if (!status.ok()) {
        #ifdef DEBUG
            printf("Returning -EIO\n\n");
        #endif
        return -EIO; // Error during deletion
    }

    std::string parentDirStr = hfs::path::returnParentDir(path);
    const char* parentDir = parentDirStr.c_str();

    // Update parent's metadata
    HFSInodeKey parentDirKey = hfs::inode::getKeyfromPath(parentDir);
    struct HFSFileMetaData parentMetaData = hfs::db::getMetaDatafromKey(db, parentDirKey);
    parentMetaData.file_structure.st_nlink--; 
    hfs::db::updateMetaData(db, parentDirKey, parentMetaData.file_structure);


    #ifdef DEBUG
        printf("Returning 0\n\n");
    #endif

    return 0;

}

int hfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    #ifdef DEBUG
        printf("read called with path %s and size: %u and offset %ld\n", path,size,offset);
    #endif

    //Fetch file -> either fi or helper functions
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFSInodeKey key;

    /*

    if(fi->fh == 0){
        uint64_t parentInode = hfs::inode::getParentInodeNumber(path);
        key = hfs::inode::getKeyFromPath(path,parentInode);
    }else{
        key.inode_number = fi->fh;
        std::string fileName = hfs::path::returnFilenameFromPath(path);
        key.inode_number_hashed = generateHash(path,fileName.length(),123);
    }
    */

    uint64_t parentInode = hfs::inode::getParentInodeNumber(path);
    key = hfs::inode::getKeyFromPath(path,parentInode);

    if(!hfs::db::keyExists(key,db)){
        #ifdef DEBUG
            printf("Returning -ENOENT\n\n");
        #endif
        return -ENOENT;
    }

    std::string data;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(),hfs::db::getKeySlice(key),&data);

    #ifdef DEBUG
        if(!status.ok()){
            printf("Error retrieving value. Error: %s\n", status.ToString());
        }
    #endif

    HFSInodeValueSerialized inodeData;
    inodeData.data = &data[0]; 
    inodeData.size = data.size();
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);

    #ifdef DEBUG
        // printf("Filename length: %zu\n",inodeValue->filename_len);
        // char* a = new char[inodeValue->filename_len];
        // memcpy(a,inodeData.data + HFS_INODE_VALUE_SIZE,inodeValue->filename_len);
        // printf("Filename: %s\n",a);
        // size_t finalSize = inodeData.size - HFS_INODE_VALUE_SIZE - inodeValue->filename_len - 1;
        // char* finalData = new char[finalSize];
        // memcpy(finalData,inodeData.data + HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1,finalSize);
        // finalData[finalSize] = '\0';
        
        // printf("Final data: %s\n",finalData);
        // printf("Final data size %zu\n",finalSize);
    #endif

    if(size > inodeValue->file_structure.st_size){size = inodeValue->file_structure.st_size;}
    char* fileptr = inodeData.data + HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1;

    #ifdef DEBUG
        printf("Data to be read:\n");
        fwrite(fileptr, 1, size, stdout);
        printf("\n");
    #endif
    memcpy(buf,fileptr,size);


    //Check if fi.fh is = 0 -> not init else it holds the parentinode number

    #ifdef DEBUG
        printf("Returning (size): %ld\n\n",size);
    #endif

    return inodeValue->file_structure.st_size;
}

int hfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    printf("Write is called with path %s and size: %ld\n", path,size);

    //Fetch file -> either fi or helper functions
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFSInodeKey key;


    // if(fi->fh == 0){
    //     printf("Fi exists with value: %d\n",fi->fh);
    //     uint64_t parentInode = hfs::inode::getParentInodeNumber(path);
    //     key = hfs::inode::getKeyFromPath(path,parentInode);
    // }else{
    //     printf("Fi not init\n");
    //     key.inode_number = fi->fh;
    //     std::string fileName = hfs::path::returnFilenameFromPath(path);
    //     key.inode_number_hashed = generateHash(path,fileName.length(),123);
    // }

    uint64_t parentInode = hfs::inode::getParentInodeNumber(path);
    key = hfs::inode::getKeyFromPath(path,parentInode);

    #ifdef DEBUG
        printf("Parent Inode: %llu\n",    key.inode_number);
    #endif

    if(!hfs::db::keyExists(key,db)){
        #ifdef DEBUG
            printf("Returning -ENOENT\n\n");
        #endif
        return -ENOENT;
    }

    std::string data;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(),hfs::db::getKeySlice(key),&data);


    if(!status.ok()){
        printf("Error retrieving value. Error: %s\n", status.ToString());
    }

    HFSInodeValueSerialized inodeData;
    inodeData.data = &data[0]; 
    inodeData.size = data.length();
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
    inodeValue->file_structure.st_size = size;

    char* filename = new char[inodeValue->filename_len + 1];
    memcpy(filename,inodeData.data + HFS_INODE_VALUE_SIZE,inodeValue->filename_len);
    filename[inodeValue->filename_len] = '\0';

    
    #ifdef DEBUG
        std::string fileStr(filename);
        printf("Filename: %s\n",fileStr.c_str());
    #endif

    //For now let's assume file size is 0 and 0 < size < T bytes will be written

    HFSInodeValueSerialized newInodeData;
    size_t memoryNeeded = HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1 + size ;
    char* newData = new char[memoryNeeded];

    memcpy(newData,inodeData.data,HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1); //Copy metadata

    char* dataBuf = newData + HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1;
    memcpy(dataBuf,buf,size);

    newInodeData.data = newData;
    newInodeData.size = memoryNeeded;

    rocksdb::Slice valueSlice = hfs::db::getValueSlice(newInodeData);
    rocksdb::Slice keySlice = hfs::db::getKeySlice(key);
    rocksdb::Status s = db->Put(rocksdb::WriteOptions(),keySlice,valueSlice);

    char* test = new char[size + 1]; // Allocate memory for test buffer
    memcpy(test, newData + HFS_INODE_VALUE_SIZE + inodeValue->filename_len + 1, size);
    test[size] = '\0'; // Null-terminate the test string
    printf("Test: %s\n", test);
    delete[] test; 

    #ifdef DEBUG
        if(!s.ok()){
            printf("Error inserting updated value with error: %s\n",s.ToString().c_str());
        }

        printf("Returning (size written): %zu\n\n",size);
    #endif

    return size;
}


int hfs_open(const char *path, struct fuse_file_info *fi){
    #ifdef DEBUG
        printf("open called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}

int hfs_truncate(const char *path, off_t len){
    #ifdef DEBUG
        printf("truncate called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}

int hfs_flush(const char *path, struct fuse_file_info *){
    #ifdef DEBUG
        printf("flush called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}

int hfs_fallocate(const char *path, int mode, off_t offset, off_t len, struct fuse_file_info *fi){
    #ifdef DEBUG
        printf("fallocate called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}

int hfs_release(const char *path, struct fuse_file_info *){
    #ifdef DEBUG
        printf("release called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}


int hfs_fsync(const char *path, int, struct fuse_file_info *){
    #ifdef DEBUG
        printf("fsync called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif
    return 0;
}

