#ifndef HFS_ROCKSDB_H
#define HFS_ROCKSDB_H

#include "hfs_inode.h"
#include "hfs_KeyHandler.h"
#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <vector>

class HFSRocksDB {
public:

    HFSRocksDB(const HFSRocksDB&) = delete;
    HFSRocksDB& operator=(const HFSRocksDB&) = delete;

    static HFSRocksDB* getInstance();

    int initDatabase(const std::string& metaDataDir);
    void initOptions();
    void initWriteOptions();
    void initReadOptions();
    void printStatusErr(rocksdb::Status status);
    int put(const HFS_KEY key, const HFSInodeValueSerialized& value);
    int get(const HFS_KEY key, std::string& value);
    int remove(const HFS_KEY key);
    rocksdb::Slice getValueSlice(const HFSInodeValueSerialized& value);
    void cleanup();

    int updateMetaData(HFS_KEY key, struct stat new_stat);
    int incrementDirLinkCount(HFS_KEY parentKey, HFS_KEY key);
    int deleteDirEntry(HFS_KEY parentKey, HFS_KEY key);
    int setBlob(HFS_KEY key);
    int setNewFileSize(HFS_KEY key,size_t size);

    HFSFileMetaData getFileHeader(HFS_KEY key);
    std::vector<HFS_KEY> getDirEntries(HFS_KEY key);
    std::string getFilename(HFS_KEY key);
    struct stat getMetaData(HFS_KEY key);

private:
    static HFSRocksDB* SingletonRocksDB;
    rocksdb::Options options;
    rocksdb::WriteOptions writeOptions;
    rocksdb::ReadOptions readOptions;
    rocksdb::BlockBasedTableOptions tableOptions;

    HFSRocksDB();
    ~HFSRocksDB();

protected:
    rocksdb::DB* db;
};

#endif 
