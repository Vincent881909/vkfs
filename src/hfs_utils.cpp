#include "../include/hfs_utils.h"
#include "../include/hash.h"
#include "../include/hfs_inode.h"
#include "../include/hfs_state.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <filesystem>

void printMetaData(rocksdb::DB* db, const HFSInodeKey key){
    printf("\n\n");
    printf("Printing MetaData...\n");

    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0]; 
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        //printStatStructure(inodeValue->file_structure);
        printf("\n");
        const char* filename = valueData.c_str() + sizeof(HFSFileMetaData);

        std::cout << "Filename: " << filename << std::endl;

    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }

}

std::string getFileNamefromKey(rocksdb::DB* db,struct HFSInodeKey key){
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {

        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];  
        inodeData.size = valueData.size();

        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        printStatStructure(inodeValue->file_structure);
        printf("\n");

        const char* filename = valueData.c_str() + sizeof(HFSFileMetaData);
        
        return filename;
    }
}

HFSInodeKey getKeyfromPath(const char* path){
    printf("Retrieve Key function called for path %s\n",path);
    struct HFSInodeKey key;
    key.inode_number = getParentInodeNumber(path);
    std::string filename = returnFilenameFromPath(path);
    const char* filenameCstr = filename.c_str();
    key.inode_number_hashed = murmur64(path, strlen(filenameCstr), 123);
    printf("Inode Number retrieved for path: %s, :%d\n",path,key.inode_number);
    printf("Inode Number Hashed retrieved for path: %s, :%llu\n",path,key.inode_number_hashed);
    return key;
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

 

struct stat getFileStat(rocksdb::DB* metaDataDB,HFSInodeKey key){
    printf("\n\n");
    printf("Rretrieve Stat is called...\n");
    std::string valueData;

    printf("Inode Number Hashed:%llu\n",key.inode_number_hashed);
    printf("Inode Number:%ld\n",key.inode_number);

    rocksdb::Status status = metaDataDB->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(valueData.data());
        return inodeValue->file_structure;
    } else {
        printf("Error retrieving stat structure\n");
    }
}

void setInodeKey(const char* path, int len, struct HFSInodeKey &inode, uint64_t maxInoderNumber) {
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

HFSInodeValueSerialized initInodeValue(struct stat fileStructure, std::string filename,mode_t mode){
    printf("\n\n");
    printf("initNodeValue is called...\n");
    struct HFSInodeValueSerialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    struct HFSFileMetaData* inode_value = reinterpret_cast<struct HFSFileMetaData*>(inode_data.data);
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

    std::filesystem::remove_all(db_path); //Erase existing Database entries upon runtime

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

std::string returnParentDir(const std::string& path) {
    if (path == "/") {return "/";} //Root

    size_t lastSlash = path.find_last_of("/");
    
    if (lastSlash == std::string::npos) {return ".";} 
    else if (lastSlash == 0) {return "/";}

    return path.substr(0, lastSlash);
}

bool pathExists(const char* path,const char* parentPath){
    struct HFSInodeKey key;
    key.inode_number_hashed = murmur64(parentPath,strlen(parentPath),123);

    // TODO

    return true;
}

std::string returnFilenameFromPath(const std::string& path) {
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

rocksdb::Slice getKeySlice(const HFSInodeKey& key){
    return rocksdb::Slice(reinterpret_cast<const char*>(&key), sizeof(HFSInodeKey));
}

rocksdb::Slice getValueSlice(const HFSInodeValueSerialized value){
    return rocksdb::Slice(value.data, value.size);
}


bool keyExists(HFSInodeKey key,rocksdb::DB* db){
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &value);
    if (s.ok()) {return true;} 
    else if (s.IsNotFound()) {return false;}
}

void printValueForKey(rocksdb::DB* db, const HFSInodeKey& key) {

    printf("Key inode: %d \n",key.inode_number);
    printf("Key inode hashed: %llu \n",key.inode_number_hashed);

    // Retrieve the value from the database
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        
        // Print the retrieved inode value
        std::cout << "Retrieved inode value for key (" << key.inode_number << ", " << key.inode_number_hashed << "):" << std::endl;
        printInodeValue(*inodeValue);  // Function to print details of inode value

    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }
}

void printInodeValue(HFSFileMetaData &inodeValue){
    std::cout << "Filename Length: " << inodeValue.filename_len << std::endl
              << "Has External Data: " << (inodeValue.has_external_data ? "Yes" : "No") << std::endl;
    std::cout << "File structure details:" << std::endl;
    printStatStructure(inodeValue.file_structure);
}

struct stat returnStatFromKey(rocksdb::DB* db, const HFSInodeKey key){
    // Retrieve the value from the database
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        return inodeValue->file_structure;


    } else if (status.IsNotFound()) {
        std::cerr << "Key not found in database." << std::endl;
    } else {
        std::cerr << "Failed to retrieve the key: " << status.ToString() << std::endl;
    }

}

struct HFSFileMetaData getMetaDatafromKey(rocksdb::DB* db, const HFSInodeKey key){
    std::string valueData;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), getKeySlice(key), &valueData);

    if (status.ok()) {
        // Assuming valueData is directly deserializable to hfs_inode_value_serialized
        HFSInodeValueSerialized inodeData;
        inodeData.data = &valueData[0];  // Point to the data in the retrieved string
        inodeData.size = valueData.size();

        // Deserialize the inode value (assuming data is stored in expected format)
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(inodeData.data);
        return *inodeValue;


    } 
}

void updateMetaData(rocksdb::DB* db, struct HFSInodeKey key, std::string filename, struct HFSFileMetaData inode_value,struct stat new_stat){
    inode_value.file_structure = new_stat;

    struct HFSInodeValueSerialized inode_data;

    inode_data.size = HFS_INODE_VALUE_SIZE + filename.length() + 1;
    inode_data.data = new char[inode_data.size];

    struct HFSFileMetaData* new_inode_value = reinterpret_cast<struct HFSFileMetaData*>(inode_data.data);
    new_inode_value->file_structure = new_stat;
    new_inode_value->filename_len = inode_value.filename_len;
    new_inode_value->has_external_data = inode_value.has_external_data;
    char* name_buf = inode_data.data + HFS_INODE_VALUE_SIZE;
    memcpy(name_buf, filename.c_str(), filename.length());  
    name_buf[inode_value.filename_len] = '\0';

    HFSInodeValueSerialized value = inode_data;

    rocksdb::Slice dbKey = getKeySlice(key);
    rocksdb::Slice dbValue = getValueSlice(value);

    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), dbKey, dbValue);


    if (status.ok()) {
        std::cout << "MetaData Update successful" << std::endl;
    } else {
        std::cerr << "MetaData Update failed: " << status.ToString() << std::endl;
    }
}

HFSInodeKey getKeyFromPath(const char* path, uint64_t parent_inode) {
    HFSInodeKey key;
    key.inode_number = parent_inode;
    key.inode_number_hashed = murmur64(path, strlen(path), 123);
    return key;
}

std::string getParentPath(const std::string& path) {
    // Edge case: if the path is root ("/"), return "/"
    if (path == "/") {
        return "/";
    }

    // Remove any trailing slashes from the path
    size_t pathLength = path.length();
    while (pathLength > 1 && path[pathLength - 1] == '/') {
        --pathLength;
    }

    // Find the position of the last slash
    size_t lastSlashPos = path.find_last_of('/', pathLength - 1);

    // Edge case: if there is no slash or the only slash is the first character, return "/"
    if (lastSlashPos == std::string::npos) {
        return ".";
    }

    if (lastSlashPos == 0) {
        return "/";
    }

    // Return the substring from the start to the last slash
    return path.substr(0, lastSlashPos);
}

int getParentInodeNumber(const char* path) {
    printf("Get Parent Inode Number function called for path: %s\n", path);
    
    if (strcmp(path, "/") == 0) { //Root
        printf("The path is root.\n");
        return ROOT_INODE_ID;
    }

    std::string pathStr(path);
    std::string parentDirStr = returnParentDir(pathStr);
    
    if (parentDirStr == "/") { //Parent is root
        printf("Parent directory is root.\n");
        return ROOT_INODE_ID;
    }

    std::string parentPath = getParentPath(pathStr);
    printf("getParentPath returned: %s\n",parentPath.c_str());

    // Split parent directory into components
    std::vector<std::string> pathComponents;

    size_t pos = 0;
    while ((pos = parentPath.find('/')) != std::string::npos) {
        if (pos != 0) {
            pathComponents.push_back(parentPath.substr(0, pos));
        }
        parentPath.erase(0, pos + 1);
    }
    if (!parentPath.empty()) {
        pathComponents.push_back(parentPath);
    }


    printf("Components in vector: ");
    printf("Component size %d \n",pathComponents.size());
    for(int i = 0; i < pathComponents.size();i++){
        printf("%s\n",pathComponents[i].c_str());
    }


    uint64_t parent_inode = ROOT_INODE_ID;
    HFSInodeKey key;
    key.inode_number = parent_inode;

    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    rocksdb::DB* metaDataDB = hfsState->getMetaDataDB();
    std::string currentPath = "/";

    for (const std::string& component : pathComponents) {
        printf("Entering for loop...\n");
        printf("Current path: %s\n",currentPath.c_str());
        currentPath = currentPath + component;
        printf("Currently looking at this component: %s\n", component.c_str());
        printf("Current path after concatenation: %s\n",currentPath.c_str());
        key = getKeyFromPath(currentPath.c_str(), parent_inode);
        key.inode_number_hashed = murmur64(currentPath.c_str(), strlen(component.c_str()), 123);

    
        if (!keyExists(key, metaDataDB)) {
            printf("Component %s not found!\n", component.c_str());
            return -1; //File or directory does not exist in MetaDataDB
        }

        struct HFSFileMetaData inodeValue = getMetaDatafromKey(metaDataDB, key);
        printf("Inode returned for current component: %d\n",inodeValue.file_structure.st_ino);
        parent_inode = inodeValue.file_structure.st_ino;
        currentPath = currentPath + "/";
    }

    printf("Parent inode number retrieved for path: %s, :%lu\n", path, parent_inode);
    return parent_inode;
}


uint64_t getInodeFromPath(const char* path,rocksdb::DB* db,std::string filename){

    printf("\n\n");
    printf("Entered getInodeFromPath with path %s\n", path);

    if(strcmp(path,ROOT_INODE) == 0){ //Is root
        printf("Rode Node ID returned\n");
        return ROOT_INODE_ID;
    }

    uint64_t parentInode = getParentInodeNumber(path);

    HFSInodeKey start_key = { parentInode, 0 };
    HFSInodeKey end_key = { parentInode + 1, 0 };

    rocksdb::ReadOptions options;
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));

    for (it->Seek(getKeySlice(start_key)); it->Valid() && it->key().compare(getKeySlice(end_key)) < 0; it->Next()) {
 
        HFSFileMetaData* inodeValue = reinterpret_cast<HFSFileMetaData*>(const_cast<char*>(it->value().data()));

        const char* currentFilename = it->value().data() + HFS_INODE_VALUE_SIZE;

        if(strcmp(currentFilename,filename.c_str()) == 0){
            printf("Filename found.Inode returned: %d\n", inodeValue->file_structure.st_ino);
            return inodeValue->file_structure.st_ino;
        }
    }

    if (!it->status().ok()) {
        std::cerr << "Iterator error: " << it->status().ToString() << std::endl;
    }
}

rocksdb::DB* getDBPtr(struct fuse_context* context){
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(context->private_data);
    return hfsState->getMetaDataDB();
}







