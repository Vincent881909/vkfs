#define FUSE_USE_VERSION 26

#include <iostream>
#include <fuse.h>
#include "hfs.h"


struct fuse_operations hfs_oper = {
  .getattr = hfs_getattr
}


int main(int argc, char *argv[]){

    int fuse_stat;
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &fhs_oper,);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
