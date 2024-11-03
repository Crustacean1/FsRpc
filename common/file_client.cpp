#include "file_client.h"
#include "file_server.h"
#include "fuse.h"
#include "stream.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <syslog.h>

stream_context *context;

void client::init(stream_context *ctx) {
  syslog(LOG_INFO, "Initializing context");
  context = ctx;
}

int client::open(const char *path, fuse_file_info *fi) {
  syslog(LOG_INFO, "Sending message");
  int64_t len = 2137;
  write(context, &len, sizeof(int64_t));
  syslog(LOG_INFO, "Opening some file");
  int res;

  res = ::open(path, fi->flags);
  if (res == -1)
    return -errno;

  /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
  parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
  for writes to the same file). */
  if (fi->flags & O_DIRECT) {
    fi->direct_io = 1;
    fi->parallel_direct_writes = 1;
  }

  fi->fh = res;
  return 0;
}

int client::getattr(const char *path, struct stat *stbuf, fuse_file_info *fi) {
  std::cout << "Getting dem attributes";

  (void)fi;
  int res;

  res = lstat(path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

int client::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                    off_t offset, struct fuse_file_info *fi,
                    enum fuse_readdir_flags flags) {
  DIR *dp;
  struct dirent *de;

  (void)offset;
  (void)fi;
  (void)flags;

  dp = opendir(path);
  if (dp == NULL)
    return -errno;

  fuse_fill_dir_flags fill_dir_plus = (fuse_fill_dir_flags)0;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0, fill_dir_plus))
      break;
  }

  closedir(dp);
  return 0;
}

int client::read(const char *path, char *buf, size_t size, off_t offset,
                 fuse_file_info *fi) {
  int fd;
  int res;

  if (fi == NULL)
    fd = ::open(path, O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}
int client::write(const char *path, const char *buf, size_t size, off_t offset,
                  fuse_file_info *fi) {
  int fd;
  int res;

  (void)fi;
  if (fi == NULL)
    fd = ::open(path, O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}
