#ifndef FILE_CLIENT_H
#define FILE_CLIENT_H

#define FUSE_USE_VERSION 31

#include "fuse.h"
#include "stream.h"

namespace client {
void init(stream_context *ctx);
int open(const char *filename, fuse_file_info *);
int getattr(const char *filename, struct stat *, fuse_file_info *);
int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int read(const char *, char *, size_t, off_t, fuse_file_info *);
int write(const char *, const char *, size_t, off_t, fuse_file_info *);
} // namespace client

#endif /*FILE_CLIENT_H*/
