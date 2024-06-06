#include "../include/hfs_rocksdb.h"
#include "../include/hfs_utils.h"
#include "../include/hfs_inode.h"
#include "rocksdb/db.h"
#include <filesystem>
#include <iostream>

HFSRocksDB* HFSRocksDB::SingletonRocksDB = nullptr;

HFSRocksDB::HFSRocksDB() : db(nullptr) {}

HFSRocksDB::~HFSRocksDB() {
    if (db) {
        delete db;
    }
}

HFSRocksDB* HFSRocksDB::getInstance() {
    if (SingletonRocksDB == nullptr) {
        SingletonRocksDB = new HFSRocksDB();
    }
    return SingletonRocksDB;
}

int HFSRocksDB::initDatabase(const std::string& dataBaseDirectory) {
    if (db) {
        printf("Database already initialzed\n");
        return -1;
    }

    int ret;
    std::string dbPath = hfs::path::getCurrentPath() + "/" + dataBaseDirectory;
    std::filesystem::remove_all(dbPath);

    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &db);

    if (!status.ok()) {
        printf("Failed to open Database %s" ,status.ToString());
        ret = -1;
    } else {
        printf("Database opened successfully\n");
        ret = 0;
    }

    return ret;
}

int HFSRocksDB::get(const HFS_KEY key, std::string& value) {
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), std::to_string(key), &value);
    if (!status.ok()) {
        return -1;
    }
    return 0;
}

int HFSRocksDB::put(const HFS_KEY key, const HFSInodeValueSerialized& value) {
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), std::to_string(key), getValueSlice(value));
    if (!status.ok()) {
        return -1;
    }
    return 0;
}

rocksdb::Slice HFSRocksDB::getValueSlice(const HFSInodeValueSerialized& value) {
    return rocksdb::Slice(value.data, value.size);
}
