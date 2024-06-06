#ifndef HFS_ROCKSDB_H
#define HFS_ROCKSDB_H

#include "rocksdb/db.h"
#include <rocksdb/slice.h>
#include "hfs_inode.h"
#include "hfs_KeyHandler.h"

class HFSRocksDB {
public:

    HFSRocksDB(const HFSRocksDB&) = delete;

    HFSRocksDB& operator=(const HFSRocksDB&) = delete;

    static HFSRocksDB* getInstance();

    int initDatabase(const std::string& dataBaseDirectory);
    int put(const HFS_KEY key, const HFSInodeValueSerialized& value);
    int get(const HFS_KEY key, std::string& value);
    rocksdb::Slice getValueSlice(const HFSInodeValueSerialized& value);

private:
    static HFSRocksDB* SingletonRocksDB;

    HFSRocksDB();
    ~HFSRocksDB();

protected:
    rocksdb::DB* db;
};

#endif 
