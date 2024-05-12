#ifndef HFS_H
#define HFS_H

#include <fuse.h>

int hfs_getattr(const char *path, struct stat *statbuf);
void *hfs_init(struct fuse_conn_info *conn);
int hfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);



#endif

