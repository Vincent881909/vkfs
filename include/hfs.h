#ifndef HFS_H
#define HFS_H

#include <fuse.h>

int hfs_getattr(const char *path, struct stat *statbuf);

void *hfs_init(struct fuse_conn_info *conn);

int hfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int hfs_create(const char *, mode_t, struct fuse_file_info *);

int hfs_utimens(const char *, const struct timespec tv[2]);

int hfs_mkdir(const char *, mode_t mode);

int hfs_open(const char *path, struct fuse_file_info *fi);

int hfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int hfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int hfs_unlink(const char *path);

int hfs_rmdir(const char *path);

int hfs_truncate(const char *path, off_t len);

int hfs_flush(const char *path, struct fuse_file_info *);

int hfs_fallocate(const char *path, int, off_t, off_t, struct fuse_file_info *);

int hfs_release(const char *path, struct fuse_file_info *);

int hfs_fsync(const char *path, int, struct fuse_file_info *);

#endif

