#include "../include/hfs_fuse.h"
#include "../include/hfs_state.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_utils.h"
#include "../include/hfs_rocksdb.h"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>
#include <vector>

void *hfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    #ifdef DEBUG
        printf("init called\n");
    #endif

    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context()); 
    HFSRocksDB* db = HFSRocksDB::getInstance();

    if (!hfsState->getRootInitFlag()) { // Root directory not initialized

        db->initDatabase(hfsState->getMetaDataDir());
        HFS_KEY key;
        keyHandler->handleEntries(hfs::ROOT_PATH,key);
    
        struct stat statbuf;
        memset(&statbuf, 0, sizeof(struct stat));
        lstat(hfs::ROOT_PATH, &statbuf);
        statbuf.st_ino = hfs::ROOT_KEY;

        HFSInodeValueSerialized value = hfs::inode::initDirHeader(statbuf, hfs::ROOT_PATH, 0755);
        db->put(key,value);

        delete[] value.data;
    }

    cfg->entry_timeout = 10; //Cache directory entries for 10 seconds in the kernel
    cfg->attr_timeout = 10; //Cache attributes for 10 seconds in the Kernel
    cfg->auto_cache = 0; //Kernel auto cache invalidated by file modifications -> disabled

    return hfsState;
}

int hfs_getattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi) {

    #ifdef DEBUG
        printf("getattr called with path: %s\n", path);
    #endif

    struct fuse_context* context = fuse_get_context();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(context);
    HFSRocksDB* db = HFSRocksDB::getInstance();

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

    fileStat = db->getMetaData(key);

    fileStat.st_atim = now; // Set Access time to now
    fileStat.st_mtim = now; // Set modified time to now
    fileStat.st_gid = context->gid; // Set group id
    fileStat.st_uid = context->uid; // Set user id

    *statbuf = fileStat;

    int ret = db->updateMetaData(key,*statbuf);

    return ret;
}

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, 
                struct fuse_file_info *fi, enum fuse_readdir_flags) {

    #ifdef DEBUG
        printf("readdir called with path: %s\n", path);
    #endif

    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFSRocksDB* db = HFSRocksDB::getInstance();

    if (filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS) < 0) {
        return -ENOMEM;
    }
    if (filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS) < 0) {
        return -ENOMEM;
    }

    HFS_KEY key;
    keyHandler->getKeyFromPath(path,key);
    std::vector<HFS_KEY> entries = db->getDirEntries(key);

    for(uint32_t i = 0; i < entries.size(); i ++){
        std::string filename = db->getFilename(entries[i]);
        struct stat fileStat = db->getMetaData(entries[i]);

        if (filler(buf, filename.c_str(), &fileStat, 0, FUSE_FILL_DIR_PLUS) < 0) {  // Fill buf with metadata
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

    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFSRocksDB* db = HFSRocksDB::getInstance();

    std::string parentDirStr = hfs::path::getParentPath(std::string(path));
    const char* parentDirPath = parentDirStr.c_str();
    std::string fileName = hfs::path::returnFilenameFromPath(path);

    if(fileName.length() > HFS_MAX_FILE_LEN){
        #ifdef DEBUG
            printf("Returning -EPERM\n\n");
        #endif
        return -EPERM;
    }

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
    int ret = db->put(key,value);

    delete[] value.data;

    if(ret < 0){
        return ret;
    }

    // Update parent's metadata
    HFS_KEY parentKey;
    if(keyHandler->getKeyFromPath(parentDirPath,parentKey) < 0){
        return -1;
    }

    if(db->incrementDirLinkCount(parentKey,key) < 0){
        return -1;
    }
    
    return 0;
}

int hfs_mkdir(const char *path, mode_t mode) {
    #ifdef DEBUG
        printf("mkdir called with path: %s and mode %u\n",path,mode);
    #endif

    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFSRocksDB* db = HFSRocksDB::getInstance();

    std::string parentDirPathStr = hfs::path::getParentPath(std::string(path));
    const char* parentDirPath = parentDirPathStr.c_str();
    std::string dirNameStr = hfs::path::returnFilenameFromPath(path);

    if(dirNameStr.length() > HFS_MAX_FILE_LEN){
        #ifdef DEBUG
            printf("Returning -EPERM\n\n");
        #endif
        return -EPERM;
    }

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
    int ret = db->put(key,value);

    delete[] value.data;

    if(ret < 0){
        return ret;
    }

    // Update parent's metadata
    HFS_KEY parentKey;
    if(keyHandler->getKeyFromPath(parentDirPath,parentKey) < 0){
        return -1;
    }

    if(db->incrementDirLinkCount(parentKey,key) < 0){
        return -1;
    }
    
    return 0;
}

int hfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {

    #ifdef DEBUG
        printf("utimens called with path %s\n", path);
    #endif

    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    struct stat metaData = db->getMetaData(key);
    metaData.st_atim = tv[0];
    metaData.st_mtim = tv[1];

    return db->updateMetaData(key,metaData);
}

int hfs_unlink(const char *path){

    #ifdef DEBUG
        printf("unlink called with path %s\n", path);
    #endif

    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // File does not exist
    }

    if(db->remove(key) < 0){
        return -1;
    }

    keyHandler->handleErase(path,key);

    // Update parent's metadata field
    HFS_KEY parentKey;
    std::string parentDirPath = hfs::path::getParentPath(std::string(path));
    if(keyHandler->getKeyFromPath(parentDirPath.c_str(),parentKey) < 0){
        return -1;
    }

    return db->deleteDirEntry(parentKey,key);

}

int hfs_rmdir(const char *path){
    #ifdef DEBUG
        printf("rmdir called with path %s\n",path);
    #endif

    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    if(db->remove(key) < 0){
        return -1;
    }

    keyHandler->handleErase(path,key);

    //Update parent's metadata field
    HFS_KEY parentKey;
    std::string parentDirPath = hfs::path::getParentPath(std::string(path));

    if(keyHandler->getKeyFromPath(parentDirPath.c_str(),parentKey) < 0){
        return -1;
    }

    return db->deleteDirEntry(parentKey,key);
}

int hfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

    #ifdef DEBUG
        printf("read called with path %s and size: %u and offset %ld\n", path,size,offset);
    #endif
    
    //Fetch file -> either fi or helper functions
    struct fuse_context* context = fuse_get_context();
    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(context);
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
    return -ENOENT; // Directory does not exist
    }
    
    std::string data;
    if(db->get(key,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0]; 
    headerValue.size = data.size();
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(headerValue.data + HFS_FLAG_SIZE);

    if(inodeValue->has_external_data){
        return localFS::readFromLocalFile(path,buf,size,offset,key,hfsState->getDataDir());
    }

    if(size > inodeValue->file_structure.st_size){
        size = size_t(inodeValue->file_structure.st_size);
    }

    char* dataFieldptr = headerValue.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1;
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
    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    HFSFileMetaData header = db->getFileHeader(key);

    if(header.has_external_data){
        int bytesWritten = localFS::writeToLocalFile(path,buf,size,offset,key,hfsState->getDataDir());
        db->setBlob(key);
        db->setNewFileSize(key,size + offset);
        return  bytesWritten;
    }

    if(size + offset > hfsState->getDataThreshold()){
        localFS::migrateAndCleanData(key,hfsState->getDataDir(),header,path);
        return localFS::writeToLocalFile(path,buf,size,offset,key,hfsState->getDataDir());
    }

    std::string data;
    if(db->get(key,data) < 0){
        return -1;
    }

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.length();

    char* newData = nullptr;
    hfs::inode::inlineWrite(headerValue, buf, size, newData);
    db->put(key,headerValue);

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

int hfs_truncate(const char *path, off_t len, struct fuse_file_info *fi){
    #ifdef DEBUG
        printf("truncate called with path %s\n",path);
        printf("Returning 0\n\n");
    #endif

    HFSRocksDB* db = HFSRocksDB::getInstance();
    HFS_KeyHandler* keyHandler = hfs::getKeyHandler(fuse_get_context());
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    HFS_KEY key;

    if(keyHandler->getKeyFromPath(path,key) < 0){
        #ifdef DEBUG
            printf("Returning -ENONENT\n\n");
        #endif
        return -ENOENT; // Directory does not exist
    }

    HFSFileMetaData header = db->getFileHeader(key);

    if(len < hfsState->getDataThreshold() && !header.has_external_data){
        hfs::inode::truncateHeaderFile(key,len,header.file_structure.st_size);
        return 0;
    }

    std::string fullFilePath = hfs::path::getLocalFilePath(key,hfsState->getDataDir());

    if(!header.has_external_data){
        localFS::migrateAndCleanData(key,hfsState->getDataDir(),header,path);
    }

    header.file_structure.st_size = len;
    db->updateMetaData(key,header.file_structure);

    return truncate(fullFilePath.c_str(),len);
}

void hfs_destroy(void *private_data){
     #ifdef DEBUG
        printf("destroy called\n");
    #endif

    HFSRocksDB* db = HFSRocksDB::getInstance();
    db->cleanup();
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


