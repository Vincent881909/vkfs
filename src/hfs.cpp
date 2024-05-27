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

void *hfs_init(struct fuse_conn_info *conn) {
    #ifdef DEBUG
        printf("init called\n");
    #endif

    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context()); 
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());

    if (!hfsState->getRootInitFlag()) { // Root directory not initialized

        HFS_KEY key;
        keyHandler->handleEntries(hfs::ROOT_PATH,key);
    
        struct stat statbuf;
        memset(&statbuf, 0, sizeof(struct stat));
        lstat(hfs::ROOT_PATH, &statbuf);
        statbuf.st_ino = hfs::ROOT_KEY;

        HFSInodeValueSerialized value = hfs::inode::initDirHeader(statbuf, hfs::ROOT_PATH, 0755);
        rocksdb::Slice dbValue = hfs::db::getValueSlice(value);
        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), dbValue);

        delete[] value.data;
    }

    return hfsState;
}

int hfs_getattr(const char *path, struct stat *statbuf) {

    #ifdef DEBUG
        printf("getattr called with path: %s\n", path);
    #endif

    struct fuse_context* context = fuse_get_context();
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(context);
    rocksdb::DB* db = hfs::db::getDBPtr(context);

    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }
    
    struct stat fileStat;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    memset(statbuf, 0, sizeof(struct stat));

    fileStat = hfs::inode::getFileStat(db, key);

    fileStat.st_atim = now; // Set Access time to now
    fileStat.st_mtim = now; // Set modified time to now
    fileStat.st_gid = context->gid; // Set group id
    fileStat.st_uid = context->uid; // Set user id

    *statbuf = fileStat;

    hfs::db::updateMetaData(db, key,*statbuf);

    return 0;
}

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

    #ifdef DEBUG
        printf("readdir called with path: %s\n", path);
    #endif

    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());

    if (filler(buf, ".", NULL, 0) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0) < 0) {
        return -ENOMEM;
    }

    HFS_KEY dirKey;
    keyHandler->getKeyFromPath(path,dirKey);
    std::vector<HFS_KEY> entries = hfs::db::getDirEntries(db,dirKey);

    for(uint32_t i = 0; i < entries.size(); i ++){
        std::string filename = hfs::db::getFileNamefromKey(db,entries[i]);
        struct stat fileStat = hfs::inode::getFileStat(db,entries[i]);

        if (filler(buf, filename.c_str(), &fileStat, 0) < 0) {         // Fill buf with metadata
        #ifdef DEBUG
            printf("Returning -ENOMEM\n\n");
        #endif  
            return -ENOMEM;
        }
    }

    return 0;
}

int hfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    #ifdef DEBUG
        printf("create called with path: %s and mode %u\n",path,mode);
    #endif

    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());

    std::string parentDirStr = hfs::path::getParentPath(std::string(path));
    const char* parentDirPath = parentDirStr.c_str();
    std::string fileName = hfs::path::returnFilenameFromPath(path);

    HFS_KEY key;

    if(keyHandler->handleEntries(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -EXISTS\n\n");
        #endif
        return -EEXIST;
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = ino_t(key);
    statbuf.st_mode = S_IFREG | mode;

    HFSInodeValueSerialized value = hfs::inode::initFileHeader(statbuf, fileName, mode);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), hfs::db::getValueSlice(value));

    delete[] value.data;

    // Update parent's metadata
    HFS_KEY parentKey;
    keyHandler->getKeyFromPath(parentDirPath,parentKey);
    hfs::db::incrementParentDirLink(db,parentKey,key);
    
    return 0;
}

int hfs_mkdir(const char *path, mode_t mode) {
    #ifdef DEBUG
        printf("mkdir called with path: %s and mode %u\n",path,mode);
    #endif

    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());

    std::string parentDirPathStr = hfs::path::getParentPath(std::string(path));
    const char* parentDirPath = parentDirPathStr.c_str();
    std::string dirNameStr = hfs::path::returnFilenameFromPath(path);

    HFS_KEY key;

    if(keyHandler->handleEntries(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -EXISTS\n\n");
        #endif
        return -EEXIST;
    }

    struct stat statbuf;
    memset(&statbuf, 0, sizeof(struct stat));
    statbuf.st_ino = ino_t(key);
    statbuf.st_mode = S_IFDIR | mode;

    HFSInodeValueSerialized value = hfs::inode::initDirHeader(statbuf, dirNameStr, mode);
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), hfs::db::getValueSlice(value));

    delete[] value.data;

    // Update parent's metadata -> Can put this into one function
    HFS_KEY parentKey;
    keyHandler->getKeyFromPath(parentDirPath,parentKey);
    hfs::db::incrementParentDirLink(db,parentKey,key);
    
    return 0;
}

int hfs_utimens(const char *path, const struct timespec tv[2]) {

    #ifdef DEBUG
        printf("utimens called with path %s\n", path);
    #endif

    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    struct stat fileStructure = hfs::inode::getFileStat(db, key);
    fileStructure.st_atim = tv[0];
    fileStructure.st_mtim = tv[1];
    hfs::db::updateMetaData(db, key, fileStructure);

    return 0;
}

int hfs_unlink(const char *path){

    #ifdef DEBUG
        printf("unlink called with path %s\n", path);
    #endif

    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), std::to_string(key));
    keyHandler->handleErase(path,key);

    // Update parent's metadata field
    HFS_KEY parentKey;
    std::string parentDirPath = hfs::path::getParentPath(std::string(path));
    keyHandler->getKeyFromPath(parentDirPath.c_str(),parentKey);
    hfs::db::deleteEntryAtParent(db,parentKey,key);

    return 0;

}

int hfs_rmdir(const char *path){
    #ifdef DEBUG
        printf("rmdir called with path %s\n",path);
    #endif

    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), std::to_string(key));
    keyHandler->handleErase(path,key);

    //Update parent's metadata field
    HFS_KEY parentKey;
    std::string parentDirPath = hfs::path::getParentPath(std::string(path));
    keyHandler->getKeyFromPath(parentDirPath.c_str(),parentKey);
    hfs::db::deleteEntryAtParent(db,parentKey,key);

    return 0;
}

int hfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    #ifdef DEBUG
        printf("read called with path %s and size: %u and offset %ld\n", path,size,offset);
    #endif
    
    //Fetch file -> either fi or helper functions
    struct fuse_context* context = fuse_get_context();
    rocksdb::DB* db = hfs::db::getDBPtr(context);
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(context);
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
    return -ENOENT; // Directory does not exist
    }
    
    std::string data;
    if (hfs::db::getValue(db, key, data) < 0) {
        return -ENOENT;
    }

    HFSInodeValueSerialized inodeData;
    inodeData.data = &data[0]; 
    inodeData.size = data.size();
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data + HFS_FLAG_SIZE);

    if(inodeValue->has_external_data){
        return hfs::path::readFromLocalFile(path,buf,size,offset,key,hfsState->getDataDir());
    }

    if(size > inodeValue->file_structure.st_size){
        size = size_t(inodeValue->file_structure.st_size);
    }

    char* dataFieldptr = inodeData.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1;
    memcpy(buf,dataFieldptr,size);


    #ifdef DEBUG
        printf("Returning (size): %ld\n\n",size);
    #endif

    return size;
}

int hfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    #ifdef DEBUG
        printf("write called with path %s and size: %u and offset %ld\n", path,size,offset);
    #endif

    //Fetch file -> either fi or helper functions
    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    HFSFileMetaData header = hfs::inode::getFileHeader(db,key);

    if(header.has_external_data){
        return hfs::path::writeToLocalFile(path,buf,size,offset,key,hfsState->getDataDir(),db); 
    }

    if(size + offset > hfsState->getDataThreshold()){
        hfs::path::migrateAndCleanData(db,key,hfsState->getDataDir(),header,path);
        return hfs::path::writeToLocalFile(path,buf,size,offset,key,hfsState->getDataDir(),db);
    }


    std::string data;
    if (hfs::db::getValue(db, key, data) < 0) {
        return -ENOENT;
    }

    HFSInodeValueSerialized inodeData;
    inodeData.data = &data[0];
    inodeData.size = data.length();

    char* newData = nullptr;
    hfs::inode::prepareWrite(inodeData, buf, size, newData);
    hfs::db::rocksDBInsert(db,key,inodeData);

    delete[] newData;
    
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

    rocksdb::DB* db = hfs::db::getDBPtr(fuse_get_context());
    HFS_KeyHandler* keyHandler = hfs::inode::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    HFSFileMetaData header = hfs::inode::getFileHeader(db,key);

    if(len < hfsState->getDataThreshold() && !header.has_external_data){
        hfs::inode::truncateHeaderFile(db,key,len,header.file_structure.st_size);
        return 0;
    }

    std::string fullFilePath = hfs::path::getLocalFilePath(key,hfsState->getDataDir());

    if(!header.has_external_data){
        hfs::path::migrateAndCleanData(db,key,hfsState->getDataDir(),header,path);
    }

    header.file_structure.st_size = len;
    hfs::db::updateMetaData(db,key,header.file_structure);

    int ret = truncate(fullFilePath.c_str(),len);

    return ret;
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



