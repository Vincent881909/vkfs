#ifndef VKFS_ROCKSDB_H
#define VKFS_ROCKSDB_H

#include "vkfs_inode.h"
#include "vkfs_key_handler.h"
#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <vector>

class VKFSRocksDB {
public:

    VKFSRocksDB(const VKFSRocksDB&) = delete;
    VKFSRocksDB& operator=(const VKFSRocksDB&) = delete;

    static VKFSRocksDB* getInstance();

    int initDatabase(const std::string& metaDataDir);
    void initOptions();
    void printStatusErr(rocksdb::Status status);
    int put(const VKFS_KEY key, const VKFSHeaderSerialized& value);
    int get(const VKFS_KEY key, std::string& value);
    int remove(const VKFS_KEY key);
    rocksdb::Slice getValueSlice(const VKFSHeaderSerialized& value);
    void cleanup();

    int updateMetaData(VKFS_KEY key, struct stat new_stat);
    int incrementDirLinkCount(VKFS_KEY parentKey, VKFS_KEY key);
    int deleteDirEntry(VKFS_KEY parentKey, VKFS_KEY key);
    int setBlob(VKFS_KEY key);
    int setNewFileSize(VKFS_KEY key,size_t size);

    VKFSFileMetaData getFileHeader(VKFS_KEY key);
    std::vector<VKFS_KEY> getDirEntries(VKFS_KEY key);
    std::string getFilename(VKFS_KEY key);
    struct stat getMetaData(VKFS_KEY key);

private:
    static VKFSRocksDB* SingletonRocksDB;
    rocksdb::Options options;
    rocksdb::WriteOptions writeOptions;
    rocksdb::ReadOptions readOptions;
    rocksdb::BlockBasedTableOptions tableOptions;

    VKFSRocksDB();
    ~VKFSRocksDB();

protected:
    rocksdb::DB* db;
};

#endif 
