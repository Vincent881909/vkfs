#include "../include/hfs_utils.h"
#include "../include/hash.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>

void printMetaData(rocksdb::DB* db, const hfs_inode_key key){
    printf("\n\n");
    printf("Printing MetaData...\n");

    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        hfs_inode_value_serialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
        printStatStructure(inodeValue->file_structure);
        printf("\n");

        // Calculate the start of the filename within valueData
        const char* filename = valueData.c_str() + sizeof(hfs_inode_value);

        // Print the filename
        std::cout << "Filename: " << filename << std::endl;

    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }

}

std::string getFileNamefromKey(rocksdb::DB* db,struct hfs_inode_key key){
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        hfs_inode_value_serialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
        printStatStructure(inodeValue->file_structure);
        printf("\n");

        // Calculate the start of the filename within valueData
        const char* filename = valueData.c_str() + sizeof(hfs_inode_value);
        
        return filename;
    }
}

hfs_inode_key retrieveKey(const char* path){
    printf("Retrieve Key function called for path %s\n",path);
    struct hfs_inode_key key;
    if(strcmp(path,"/") == 0){  //Root directory
        key.inode_number = ROOT_INODE_ID;
        key.inode_number_hashed = murmur64(path, 0, 123);
        printf("Inode Number retrieved for path: %s, :%d\n",path,key.inode_number);
        printf("Inode Number Hashed retrieved for path: %s, :%llu\n",path,key.inode_number_hashed);
        return key;
    } 
}

void printStatStructure(const struct stat& statbuf) {
    printf("Information from struct stat:\n");
    printf("Device ID (st_dev): %lu\n", (unsigned long)statbuf.st_dev);
    printf("Inode number (st_ino): %lu\n", (unsigned long)statbuf.st_ino);
    printf("File type and mode (st_mode): %o (octal)\n", statbuf.st_mode);
    printf("Number of hard links (st_nlink): %lu\n", (unsigned long)statbuf.st_nlink);
    printf("User ID of owner (st_uid): %u\n", statbuf.st_uid);
    printf("Group ID of owner (st_gid): %u\n", statbuf.st_gid);
    printf("Device ID (if special file) (st_rdev): %lu\n", (unsigned long)statbuf.st_rdev);
    printf("Total size, in bytes (st_size): %ld\n", (long)statbuf.st_size);
    printf("Block size for filesystem I/O (st_blksize): %lu\n", (unsigned long)statbuf.st_blksize);
    printf("Number of blocks allocated (st_blocks): %lu\n", (unsigned long)statbuf.st_blocks);

    // Converting time_t to readable format
    char timeString[100];
    if (strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&statbuf.st_atime))) {
        printf("Time of last access (st_atime): %s\n", timeString);
    }
    if (strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&statbuf.st_mtime))) {
        printf("Time of last modification (st_mtime): %s\n", timeString);
    }
    if (strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&statbuf.st_ctime))) {
        printf("Time of last status change (st_ctime): %s\n", timeString);
    }
}

 

struct stat retrieveStat(rocksdb::DB* metaDataDB,hfs_inode_key key){
    printf("\n\n");
    printf("Rretrieve Stat is called...\n");
    std::string valueData;

    printf("Inode Number Hashed:%llu\n",key.inode_number_hashed);
    printf("Inode Number:%ld\n",key.inode_number);

    rocksdb::Status status = metaDataDB->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(valueData.data());
        return inodeValue->file_structure;
    } else {
        printf("Error retrieving stat structure\n");
    }
}

void createMetaDataKey(const char* path, int len, struct hfs_inode_key &inode, uint64_t maxInoderNumber) {
    // Print debug statements for all values
    printf("\n\n");
    printf("CreateMetaDataKey Function is called with parameters: \n");
    printf("Path: %s\n", path);
    printf("File length: %d\n",len);
    printf("Inode Number: %d\n", maxInoderNumber);
    printf("\n\n");

    // Set inode values
    inode.inode_number = maxInoderNumber; //Inode number  of parent directory
    inode.inode_number_hashed = murmur64(path, len, 123);

    // Print debug statements for updated inode values
    printf("Inode Number of parent: %d\n", inode.inode_number);
    printf("Inode Number Hashed: %llu\n", inode.inode_number_hashed);
    printf("\n\n");
}

void initStat(struct stat &statbuf,mode_t mode){
    printf("\n\n initStat is called...\n");
    statbuf.st_mode = mode;

    if (S_ISREG(mode)) {
        statbuf.st_nlink = 1; // Regular File
    } else if (S_ISDIR(mode)) {
        statbuf.st_nlink = 2; // Directory
    }
    
    statbuf.st_uid = 0; //To be initalized after first getAttr call
    statbuf.st_gid = 0; //To be initalized after first getAttr call
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

hfs_inode_value_serialized initInodeValue(struct stat fileStructure, std::string filename,mode_t mode){
    printf("\n\n");
    printf("initNodeValue is called...\n");
    struct hfs_inode_value_serialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    struct hfs_inode_value* inode_value = reinterpret_cast<struct hfs_inode_value*>(inode_data.data);
    initStat(fileStructure,mode);
    inode_value->file_structure = fileStructure;
    inode_value->has_external_data = false;
    inode_value->filename_len = filename.length();

    printStatStructure(inode_value->file_structure);

    char* name_buf = inode_data.data + HFS_INODE_VALUE_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());  
    name_buf[inode_value->filename_len] = '\0';

    return inode_data;
}

std::string getCurrentPath(){
  char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return "";
    }

  return std::string(cwd);
}

rocksdb::DB* createMetaDataDB(std::string metadir) {

    std::string db_path = getCurrentPath() + "/" + metadir;
    std::cout << "Path for DB: " << db_path << "\n";

    // Open the database
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        std::cerr << "Unable to open database: " << status.ToString() << std::endl;
    } else {
        std::cout<< "DB opened succefully...\n";
    }

    
  return db;

}

std::string getParentDirectory(const std::string& path) {
    if (path == "/") { // Root directory
        return "/";
    }

    size_t lastSlash = path.find_last_of("/");
    
    if (lastSlash == std::string::npos) {
        // No slash found, implying the path is a file or directory in the current directory
        return ".";
    } else if (lastSlash == 0) {
        // The path is in the root directory
        return "/";
    }

    // Return the substring from the start to the last slash
    return path.substr(0, lastSlash);
}

bool pathExists(const char* path,const char* parentPath){
    struct hfs_inode_key key;
    key.inode_number_hashed = murmur64(parentPath,strlen(parentPath),123);

    // TODO

    return true;
}

std::string get_filename_from_path(const std::string& path) {
    if (path.empty()) {
        return ""; // Return empty string for empty input
    }

    // Find the last position of '/' which marks the start of the filename
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path; // No '/' found, the entire path is a filename
    } else if (pos == path.length() - 1) {
        // Handle case where path ends with a slash
        // We need to find the beginning of the filename before the last '/'
        size_t end = pos;
        while (end > 0 && path[end - 1] == '/') {
            end--; // Skip trailing slashes
        }

        if (end == 0) {
            return ""; // Path consisted only of slashes
        }

        // Find the previous slash before the end of the filename
        pos = path.find_last_of('/', end - 1);
        if (pos == std::string::npos) {
            return path.substr(0, end); // No additional '/' found
        } else {
            return path.substr(pos + 1, end - pos - 1); // Extract the filename
        }
    } else {
        return path.substr(pos + 1); // Extract the filename after the last '/'
    }
}

rocksdb::Slice getKeySlice(const hfs_inode_key& key){
    return rocksdb::Slice(reinterpret_cast<const char*>(&key), sizeof(hfs_inode_key));
}

rocksdb::Slice getValueSlice(const hfs_inode_value_serialized value){
    return rocksdb::Slice(value.data, value.size);
}


bool keyExists(hfs_inode_key key,rocksdb::DB* db){
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &value);
    if (s.ok()) {return true;} 
    else if (s.IsNotFound()) {return false;}
}

void printValueForKey(rocksdb::DB* db, const hfs_inode_key& key) {

    printf("Key inode: %d \n",key.inode_number);
    printf("Key inode hashed: %llu \n",key.inode_number_hashed);

    // Retrieve the value from the database
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        hfs_inode_value_serialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
        
        // Print the retrieved inode value
        std::cout << "Retrieved inode value for key (" << key.inode_number << ", " << key.inode_number_hashed << "):" << std::endl;
        printInodeValue(*inodeValue);  // Function to print details of inode value

    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }
}

void printInodeValue(hfs_inode_value &inodeValue){
    std::cout << "Filename Length: " << inodeValue.filename_len << std::endl
              << "Has External Data: " << (inodeValue.has_external_data ? "Yes" : "No") << std::endl;
    std::cout << "File structure details:" << std::endl;
    printStatStructure(inodeValue.file_structure);
}

struct stat returnStatFromKey(rocksdb::DB* db, const hfs_inode_key key){
    // Retrieve the value from the database
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        hfs_inode_value_serialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
        return inodeValue->file_structure;


    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }

}

struct hfs_inode_value returnInodeValuefromKey(rocksdb::DB* db, const hfs_inode_key key){
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        hfs_inode_value_serialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        hfs_inode_value* inodeValue = reinterpret_cast<hfs_inode_value*>(inodeData.data);
        return *inodeValue;


    } 
}

void updateMetaData(rocksdb::DB* db, struct hfs_inode_key key, std::string filename, struct hfs_inode_value inode_value,struct stat new_stat){
    inode_value.file_structure = new_stat;

    struct hfs_inode_value_serialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    struct hfs_inode_value* new_inode_value = reinterpret_cast<struct hfs_inode_value*>(inode_data.data);
    new_inode_value->file_structure = new_stat;
    new_inode_value->filename_len = inode_value.filename_len;
    new_inode_value->has_external_data = inode_value.has_external_data;
    char* name_buf = inode_data.data + HFS_INODE_VALUE_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());  
    name_buf[inode_value.filename_len] = '\0';

    hfs_inode_value_serialized value = inode_data;

    rocksdb::Slice dbKey = getKeySlice(key);
    rocksdb::Slice dbValue = getValueSlice(value);

    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), dbKey, dbValue);


    if (status.ok()) {
        std::cout << "MetaData Update successful" << std::endl;
    } else {
        std::cerr << "MetaData Update failed: " << status.ToString() << std::endl;
    }
}





