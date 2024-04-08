#include <iostream>
#include <hfs.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int hfs_getattr(const char *path, struct stat *statbuf){
    int res = 0;

    memset(st, 0, sizeof(struct stat));
    /* Set owner to user/group who mounted the image */
    st->st_uid = getuid();
    st->st_gid = getgid();
    /* Last accessed/modified just now */
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        res = -2;
    }

    return res;
}