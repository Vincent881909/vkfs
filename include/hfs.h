#ifndef HFS_H
#define HFS_H

int hfs_getattr(const char *path, struct stat *statbuf);
void *hfs_init(struct fuse_conn_info *conn);

#endif

