#ifndef CLIENT_H
#define CLIENT_H

#include <thread>
#define FUSE_USE_VERSION 31

#include "fuse.h"
#include "stream.h"

#include "../common/stream.h"

template <typename R, typename... Args>
R execute(stream_context *context, uint64_t fn_type, Args... args) {
  auto request = serialize(fn_type, args...);

  syslog(LOG_INFO, " Fn: %lu %lu", fn_type,
         *(uint64_t *)((char *)(request.data()) + sizeof(uint64_t)));
  write((stream_context *)context, (void *)request.data(), (int)request.size());
  uint64_t response_length;
  read((stream_context *)context, &response_length, sizeof(uint64_t));
  std::vector<uint8_t> buffer(response_length);
  read((stream_context *)context, buffer.data(), response_length);
  auto *ptr = buffer.data();

  R result;
  read_self<R>(&ptr, result);
  return result;
}

namespace client {

struct client_context {
  stream_context *context;
  std::thread thr;
};

void *init(fuse_conn_info *, fuse_config *);
void destroy(void *);
int open(const char *filename, fuse_file_info *);
int getattr(const char *filename, struct stat *, fuse_file_info *);
int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int read(const char *, char *, size_t, off_t, fuse_file_info *);
int write(const char *, const char *, size_t, off_t, fuse_file_info *);
} // namespace client

#endif /*CLIENT_H*/
