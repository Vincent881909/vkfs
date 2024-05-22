#define FUSE_USE_VERSION 26

#include "../include/hfs.h"
#include "../include/hfs_state.h"
#include "../include/hfs_utils.h"
#include <iostream>
#include <fuse.h>
#include <assert.h>
#include <string.h>
#include "rocksdb/db.h"
#include <assert.h>
#include <unistd.h>

struct fuse_operations hfs_oper = {
  .getattr = hfs_getattr,
  .mkdir = hfs_mkdir,
  .unlink = hfs_unlink,
  .rmdir = hfs_rmdir,
  .truncate = hfs_truncate,
  .open = hfs_open,
  .read = hfs_read,
  .write = hfs_write,
  .flush = hfs_flush,
  .release = hfs_release,
  .fsync = hfs_fsync,
  .readdir = hfs_readdir,
  .init = hfs_init,
  .create = hfs_create,
  .utimens = hfs_utimens,
  .fallocate = hfs_fallocate,
};

int main(int argc, char *argv[]){

    std::string mountdir(argv[1]);
    std::string metadir(argv[2]);
    std::string datadir(argv[3]);

    char *fuse_argv[20];
    int fuse_argc = 0;
    fuse_argv[fuse_argc++] = argv[0];
    char fuse_mount_dir[100];
    strcpy(fuse_mount_dir, mountdir.c_str());
    fuse_argv[fuse_argc++] = fuse_mount_dir;
    fuse_argv[fuse_argc++] = "-f"; //Run Fuse in the Foreground
    //fuse_argv[fuse_argc++] = "-d"; //Verbose debug output
    fuse_argv[fuse_argc++] = "-s"; //Single threaded mode
    //fuse_argv[fuse_argc++] = "default_permissions"; //Single threaded mode
    fuse_argv[fuse_argc++] = "-o"; // Allow access to other users
    fuse_argv[fuse_argc++] = "allow_other"; // Allow access to other users
    int fuse_state;
    
    //u_int dataThreshold = 4096;
    u_int dataThreshold = 20000;
    rocksdb::DB* metaDataDB = hfs::db::createMetaDataDB(metadir);
    HFS_FileSystemState hfsState(mountdir,metadir,datadir,dataThreshold, metaDataDB);


    #ifdef DEBUG
      printf("Calling fuse_main...\n");
    #endif

    fuse_state = fuse_main(fuse_argc, fuse_argv, &hfs_oper, &hfsState);

    #ifdef DEBUG
      printf("fuse_main has exited with code: %d\n", fuse_state);
    #endif

    return fuse_state;
}

