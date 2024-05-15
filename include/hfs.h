#ifndef HFS_H
#define HFS_H

#include <fuse.h>

int hfs_getattr(const char *path, struct stat *statbuf);

void *hfs_init(struct fuse_conn_info *conn);

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int hfs_create(const char *, mode_t, struct fuse_file_info *);

int hfs_utimens(const char *, const struct timespec tv[2]);

int hfs_mkdir(const char *, mode_t mode);


#endif

