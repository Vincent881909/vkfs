#ifndef VKFS_FUSE_H
#define VKFS_FUSE_H

#include <fuse.h>

int vkfs_getattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi);

void *vkfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

int vkfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi, enum fuse_readdir_flags);

void vkfs_destroy(void *private_data);                

int vkfs_create(const char *, mode_t, struct fuse_file_info *);

int vkfs_utimens(const char *, const struct timespec tv[2], struct fuse_file_info *fi);

int vkfs_mkdir(const char *, mode_t mode);

int vkfs_open(const char *path, struct fuse_file_info *fi);

int vkfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int vkfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int vkfs_unlink(const char *path);

int vkfs_rmdir(const char *path);

int vkfs_truncate(const char *path, off_t len, struct fuse_file_info *fi);

int vkfs_flush(const char *path, struct fuse_file_info *);

int vkfs_fallocate(const char *path, int, off_t, off_t, struct fuse_file_info *);

int vkfs_release(const char *path, struct fuse_file_info *);

int vkfs_fsync(const char *path, int, struct fuse_file_info *);

#endif

