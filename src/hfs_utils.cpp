#include "../include/hfs_utils.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include "../include/hfs_rocksdb.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits.h>

namespace fs = std::filesystem;

namespace hfs {

HFS_KeyHandler* getKeyHandler(struct fuse_context* context){
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    return hfsState->getKeyHandler();

}

namespace inode {

void inlineWrite(HFSInodeValueSerialized& inodeData, const char* buf, size_t size, char*& newData) {
    HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data + HFS_FLAG_SIZE);
    inodeValue->file_structure.st_size = size;

    if (inodeValue->file_structure.st_size == 0 && size == 0) {
        return;
    }

    size_t memoryNeeded = HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1 + size;
    newData = new char[memoryNeeded];

    memcpy(newData, inodeData.data, memoryNeeded - size); // Copy metadata
    char* dataBuf = newData + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + inodeValue->filename_len + 1;
    memcpy(dataBuf, buf, size);

    inodeData.data = newData;
    inodeData.size = memoryNeeded;
}


void initStat(struct stat &statbuf, mode_t mode) {
    statbuf.st_mode = mode;

    if (S_ISREG(mode)) {
        statbuf.st_nlink = 1;
    } else if (S_ISDIR(mode)) {
        statbuf.st_nlink = 2;
    }

    statbuf.st_uid = 0;
    statbuf.st_gid = 0;
    statbuf.st_rdev = 0;
    statbuf.st_size = 0;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    statbuf.st_atim = now;
    statbuf.st_mtim = now;
    statbuf.st_ctim = now;
    statbuf.st_blksize = 0;
    statbuf.st_blocks = 0;
}

HFSInodeValueSerialized initDirHeader(struct stat fileStructure, std::string filename, mode_t mode){
    struct HFSInodeValueSerialized dirHeaderValue;

    dirHeaderValue.size = HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE + filename.length() + 1 ;
    dirHeaderValue.data = new char[dirHeaderValue.size];

    memcpy(dirHeaderValue.data,&HFS_DIR_FLAG,HFS_FLAG_SIZE);

    struct HFSDirMetaData* dirHeader = reinterpret_cast<struct HFSDirMetaData*>(dirHeaderValue.data + HFS_FLAG_SIZE);
    initStat(fileStructure, S_IFDIR | mode);
    dirHeader->file_structure = fileStructure;
    dirHeader->filename_len = filename.length();

    char* name_buf = dirHeaderValue.data + HFS_FLAG_SIZE + HFS_DIR_HEADER_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());
    name_buf[dirHeader->filename_len] = '\0';

    return dirHeaderValue;

}

HFSInodeValueSerialized initFileHeader(struct stat fileStructure, std::string filename, mode_t mode) {
    struct HFSInodeValueSerialized inode_data;

    inode_data.size = HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    memcpy(inode_data.data,&HFS_FILE_FLAG,HFS_FLAG_SIZE);
    
    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(inode_data.data + HFS_FLAG_SIZE);
    initStat(fileStructure, mode);
    inode_value->file_structure = fileStructure;
    inode_value->has_external_data = false;
    inode_value->filename_len = filename.length();

    char* name_buf = inode_data.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());
    name_buf[inode_value->filename_len] = '\0';

    return inode_data;
}

void truncateHeaderFile( HFS_KEY key,off_t len,size_t currentSize){
    HFSRocksDB* db = HFSRocksDB::getInstance();
    std::string data;
    db->get(key,data);

    HFSInodeValueSerialized headervalue;
    headervalue.data = &data[0];
    headervalue.size = data.size();

    if(len <= currentSize){ //Shrink File
        headervalue.size = headervalue.size - currentSize + len;
    }else{ //Extend file with null bytes
        char* newData = new char[headervalue.size - currentSize + len];
        memcpy(newData,headervalue.data,headervalue.size);
        size_t difference = len - currentSize;
        memset(newData + headervalue.size,0,difference);
        headervalue.size = headervalue.size + difference;
        headervalue.data = newData;
    }

    HFSFileMetaData* metaData = reinterpret_cast<HFSFileMetaData*>(headervalue.data + HFS_FLAG_SIZE);
    metaData->file_structure.st_size = len;

    db->put(key,headervalue);

    if(len > currentSize){
        delete[] headervalue.data;
    }
}
}

namespace path {

fs::path getExecutablePath() {
    char result[PATH_MAX];
    size_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count < 0) {
        throw std::runtime_error("Failed to read the executable path");
    }
    return fs::path(std::string(result, count));
}

std::string getProjectRoot() {
    fs::path execPath = getExecutablePath();
    std::string projectPath = std::string(execPath.parent_path().parent_path());
    return projectPath;
}

std::string returnFilenameFromPath(const std::string& path) {
    std::filesystem::path fsPath(path);
    std::filesystem::path filename = fsPath.filename();

    return filename.string();
}

std::string getParentPath(const std::string& path) {
    std::filesystem::path fsPath(path);

    if (fsPath == "/") { //path is root, no parent
        return "/";
    }

    std::filesystem::path parentPath = fsPath.parent_path();
    std::string parentDir = parentPath.string();

    if (parentDir.empty()) { //parent is root
        return "/";
    }

    return parentDir;
}

std::string getLocalFilePath(HFS_KEY key,std::string datadir){
    std::string J = std::to_string(key / 1000);
    std::string I = std::to_string(key);
    std::string projectPath = getProjectRoot();
    std::string fullPath = projectPath + "/" + datadir + "/" + J + "/" + I;

    return fullPath;
}


} // namespace path
}// namespace hfs

namespace localFS {
int writeToLocalFile(const char *path, const char *buf, size_t size, off_t offset, HFS_KEY key,std::string datadir){
    fs::path fullPath = fs::path(hfs::path::getLocalFilePath(key,datadir));

    if (!fs::exists(fullPath.parent_path())) {
        fs::create_directories(fullPath.parent_path());
    }

    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        return -1;
    }

    ssize_t bytesWritten = pwrite(fd, buf, size, offset);
    if (bytesWritten == -1) {
        close(fd);
        return -1;
    }

    close(fd);

    return bytesWritten;
}

int readFromLocalFile(const char *path, const char *buf, size_t size, off_t offset,HFS_KEY key,std::string datadir){
    fs::path fullPath = fs::path(hfs::path::getLocalFilePath(key,datadir));

    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd == -1) {
        
        return -1;
    } 

    ssize_t bytesRead = pread(fd, (void*)buf, size, offset);
    if (bytesRead == -1) {
        close(fd);
        return -1;
    } 

    close(fd);

    return bytesRead;
}

void migrateAndCleanData(HFS_KEY key,std::string datadir,HFSFileMetaData header,const char* path){
    HFSRocksDB* db = HFSRocksDB::getInstance();
    
    std::string data;
    db->get(key,data);

    HFSInodeValueSerialized headerValue;
    headerValue.data = &data[0];
    headerValue.size = data.size();

    const char* dataOffs = headerValue.data + HFS_FLAG_SIZE + HFS_FILE_HEADER_SIZE + header.filename_len + 1;

    //Copy existing data to local file system
    localFS::writeToLocalFile(path,dataOffs,header.file_structure.st_size,0,key,datadir);
    headerValue.size = headerValue.size - header.file_structure.st_size;

    db->put(key,headerValue);
    db->setBlob(key);

}   
} //namespace localFS
