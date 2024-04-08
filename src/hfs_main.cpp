#define FUSE_USE_VERSION 26

#include <iostream>
#include <fuse.h>
#include "hfs.h"
#include <assert.h>
#include <string.h>

struct fuse_operations hfs_oper = {
  .getattr = hfs_getattr
};

int main(int argc, char *argv[]){

    std::string mountdir(argv[1]);
    std::string metadir(argv[2]);
    std::string datadir(argv[3]);

    std::cout << "Mount directory: " << mountdir << "\n";
    std::cout << "Metadta directory: " << metadir << "\n";
    std::cout << "Data directory: " << datadir << "\n";

    char *fuse_argv[20];
    int fuse_argc = 0;
    fuse_argv[fuse_argc++] = argv[0];
    char fuse_mount_dir[100];
    strcpy(fuse_mount_dir, mountdir.c_str());
    fuse_argv[fuse_argc++] = fuse_mount_dir;
    fuse_argv[fuse_argc++] = "-f"; //Run Fuse in the Foreground
    fuse_argv[fuse_argc++] = "-d"; //Verbose debug output
    fuse_argv[fuse_argc++] = "-s"; //Single threaded mode
 
    int fuse_state;

    printf("Calling fuse_main...\n");
    fuse_state = fuse_main(fuse_argc, fuse_argv, &hfs_oper, NULL);
    printf("fuse_main has exited with code: %d\n", fuse_state);

    return fuse_state;
}

