#include <iostream>
#include "../include/hfs.h"
#include "../include/hfs_state.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fuse.h>

int hfs_getattr(const char *path, struct stat *statbuf){
    int res = 0;

    memset(statbuf, 0, sizeof(struct stat));
    /* Set owner to user/group who mounted the image */
    statbuf->st_uid = getuid();
    statbuf->st_gid = getgid();
    /* Last accessed/modified just now */
    statbuf->st_atime = time(NULL);
    statbuf->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0) {
        statbuf->st_mode = S_IFDIR | 0755;
        statbuf->st_nlink = 2;
    } else {
        res = -2;
    }

    return res;
}

void *hfs_init(struct fuse_conn_info *conn){
    printf("Initaliziing Hybrid File System...\n");
    /*
    HFS_FileSystemState *hfsState = static_cast<HFS_FileSystemState*>(fuse_get_context()->private_data);
    hfsState->printState();
    */


}