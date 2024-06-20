#define FUSE_USE_VERSION 31

#include "../include/vkfs_fuse.h"
#include "../include/vkfs_state.h"
#include "../include/vkfs_key_handler.h"
#include <iostream>

struct fuse_operations vkfs_operations = {
  .getattr = vkfs_getattr,
  .mkdir = vkfs_mkdir,
  .unlink = vkfs_unlink,
  .rmdir = vkfs_rmdir,
  .truncate = vkfs_truncate,
  .open = vkfs_open,
  .read = vkfs_read,
  .write = vkfs_write,
  .flush = vkfs_flush,
  .release = vkfs_release,
  .fsync = vkfs_fsync,
  .readdir = vkfs_readdir,
  .init = vkfs_init,
  .destroy = vkfs_destroy,
  .create = vkfs_create,
  .utimens = vkfs_utimens,
  .fallocate = vkfs_fallocate,
};

void vkfsUsage() {
    printf("Usage: vkfs <mount_dir> <meta_dir> <data_dir>\n");
    printf("\n");
    printf("    <mount_dir>  - Directory to mount the file system\n");
    printf("    <meta_dir>   - Directory to store metadata\n");
    printf("    <data_dir>   - Directory to store data in underlying file system\n");
    printf("\n");
    printf("Example:\n");
    printf("    ./vkfs mntdir metadir datadir\n");
}

int main(int argc, char *argv[]){
  
    if(argc < 4){
        vkfsUsage();
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
    VKFS_KeyHandler newHandler;
    VKFS_FileSystemState vkfsState(mountdir,metadir,datadir,dataThreshold,&newHandler);

    #ifdef DEBUG
      printf("Calling fuse_main...\n");
    #endif

    fuse_state = fuse_main(fuseArgs.argc, fuseArgs.argv, &vkfs_operations, &vkfsState);

    #ifdef DEBUG
      printf("fuse_main has exited with code: %d\n", fuse_state);
    #endif

    fuse_opt_free_args(&fuseArgs); //Clean up fuse args

    return fuse_state;
}

