#include "file_client.h"
#include "stream.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <syslog.h>

stream_context *context;


/*int client::open(const char *path, fuse_file_info *fi) {
  syslog(LOG_INFO, "Opening some file");
  int path_len = strlen(path) + 1;
  int length = path_len + sizeof(open_in);
  auto buffer = new uint8_t[2 * sizeof(uint64_t) + length];
  command *ptr = (command *)buffer;

  uint8_t *data = (uint8_t *)(&ptr->data.open + 1);
  memcpy(data, path, path_len);
  ptr->data.open.path = (const char *)((uint8_t *)data - (uint8_t *)ptr);
  data += path_len;
  write(context, buffer, sizeof(int64_t) + length);

  syslog(LOG_INFO, "Request sent");

  int64_t len;
  read(context, &len, sizeof(int64_t));
  auto recv_buffer = new uint8_t[len];
  open_out *recv = (open_out *)recv_buffer;
  read(context, recv_buffer, len);
  syslog(LOG_INFO, "Read response");
  memcpy(fi, &recv->fi, sizeof(fuse_file_info));

  return 0;
}

int client::getattr(const char *path, struct stat *stbuf, fuse_file_info *fi) {
  std::cout << "Getting dem attributes";
  int path_len = strlen(path) + 1;
  auto buffer = new uint8_t[sizeof(uint64_t) + sizeof(getattr_in) + path_len];
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
}*/
