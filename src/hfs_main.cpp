#define FUSE_USE_VERSION 31

#include "../include/hfs_fuse.h"
#include "../include/hfs_state.h"
#include "../include/hfs_utils.h"
#include "../include/hfs_KeyHandler.h"
#include "../include/hfs_rocksdb.h"
#include <iostream>
#include <fuse.h>
#include <assert.h>
#include <string.h>
#include "rocksdb/db.h"
#include <assert.h>
#include <unistd.h>

struct fuse_operations vkfs_operations = {
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
  .destroy = hfs_destroy,
  .create = hfs_create,
  .utimens = hfs_utimens,
  .fallocate = hfs_fallocate,
};

void hfsUsage() {
    printf("Usage: hybridfs <mount_dir> <meta_dir> <data_dir>\n");
    printf("\n");
    printf("    <mount_dir>  - Directory to mount the file system\n");
    printf("    <meta_dir>   - Directory to store metadata\n");
    printf("    <data_dir>   - Directory to store data in underlying file system\n");
    printf("\n");
    printf("Example:\n");
    printf("    ./hybridfs mntdir metadir datadir\n");
}

int main(int argc, char *argv[]){
  
    if(argc < 4){
        hfsUsage();
        return -1;
    }
 
    std::string mountdir(argv[1]);
    std::string metadir(argv[2]);
    std::string datadir(argv[3]);

    struct fuse_args fuseArgs = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&fuseArgs, argv[0]); //Progam Name
    fuse_opt_add_arg(&fuseArgs, argv[1]); //Mount Directory
    fuse_opt_add_arg(&fuseArgs, "-f"); //Run Fuse in the Foreground
    fuse_opt_add_arg(&fuseArgs, "-oallow_other"); // Allow access to other users
    //fuse_opt_add_arg(&fuseArgs, "-d"); // Debug flag
  
    int fuse_state;
    u_int dataThreshold = DATA_THRESHOLD;
    HFS_KeyHandler newHandler;
    HFS_FileSystemState hfsState(mountdir,metadir,datadir,dataThreshold,&newHandler);

    #ifdef DEBUG
      printf("Calling fuse_main...\n");
    #endif

    fuse_state = fuse_main(fuseArgs.argc, fuseArgs.argv, &vkfs_operations, &hfsState);

    #ifdef DEBUG
      printf("fuse_main has exited with code: %d\n", fuse_state);
    #endif

    fuse_opt_free_args(&fuseArgs); //Clean up fuse args

    return fuse_state;
}

