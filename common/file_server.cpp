
/*#include "file_server.h"
#include "stream.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

response *server::execute(command *command) {
  response *ptr;
  switch (command->function) {
  case 1:
    ptr = open((open_in *)command->data);
    break;
  case 2:
    ptr = read((read_in *)command->data);
    break;
  case 3:
    ptr = write((write_in *)command->data);
    break;
  case 4:
    ptr = lseek((lseek_in *)command->data);
    break;
  case 5:
    ptr = chmod((chmod_in *)command->data);
    break;
  case 6:
    ptr = unlink((unlink_in *)command->data);
    break;
  case 7:
    ptr = rename((rename_in *)command->data);
    break;
  case 8:
    ptr = getattr((getattr_in *)command->data);
    break;
  case 9:
    ptr = readdir((readdir_in *)command->data);
    break;
  default:
    std::cerr << "Invalid function specification: " << command->function
              << std::endl;
    return nullptr;
  }
  return ptr;
}

response *server::open(open_in *in) {
  auto buffer = new uint8_t[sizeof(uint64_t) + sizeof(open_out)];
  response *ptr = (response *)buffer;
  ptr->length = sizeof(open_out);

  int res = ::open(in->path, in->fi.flags);
  if (res == -1)
    return nullptr;

  /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
  parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
  for writes to the same file).
  if (in->fi.flags & O_DIRECT) {
    in->fi.direct_io = 1;
    in->fi.parallel_direct_writes = 1;
  }

  in->fi.fh = res;
  memcpy(&ptr->res.open.fi, &in->fi, sizeof(fuse_file_info));

  return ptr;
}

response *server::read(read_in *in) {
  auto length = in->size + sizeof(read_out);
  uint8_t buffer[sizeof(uint64_t) + sizeof length];
  response *ptr = (response *)buffer;

  in->fi += (uint64_t)in;
  in->path += (uint64_t)in;

  ptr->res.read.buf = &ptr->res.read + 1;
  ptr->length = length;

  int fd;
  int res;

  if (in->fi == NULL)
    fd = ::open(in->path, O_RDONLY);
  else
    fd = in->fi->fh;

  if (fd == -1)
    return nullptr;

  ptr->res.read.size = pread(fd, ptr->res.read.buf, in->size, in->offset);
  if (res == -1)
    return nullptr;

  if (in->fi == NULL)
    close(fd);

  return ptr;
}

response *server::write(write_in *in) {
  int length = sizeof(write_out);
  uint8_t *buffer = new uint8_t[sizeof(uint64_t) + length];
  response *ptr = (response *)buffer;
  ptr->length = length;

  in->path += (uint64_t)in;
  in->buf += (uint64_t)in;
  in->fi += (uint64_t)in;

  int fd;

  (void)in->fi;
  if (in->fi == NULL)
    fd = ::open(in->path, O_WRONLY);
  else
    fd = in->fi->fh;

  if (fd == -1)
    return nullptr;

  ptr->res.write.count = pwrite(fd, in->buf, in->size, in->offset);
  if (ptr->res.write.count == -1)
    ptr->res.write.count = -errno;

  if (in->fi == NULL)
    close(fd);
  return ptr;
}

response *server::lseek(lseek_in *) { return nullptr; }

response *server::chmod(chmod_in *) { return nullptr; }

response *server::unlink(unlink_in *) { return nullptr; }

response *server::rename(rename_in *) { return nullptr; }

response *server::readdir(readdir_in *in) {
  DIR *dp;
  struct dirent *de;

  (void)in->offset;
  (void)in->fi;
  (void)in->flags;

  dp = opendir(in->path);
  if (dp == NULL)
    return nullptr;

  fuse_fill_dir_flags fill_dir_plus = (fuse_fill_dir_flags)0;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (in->filler(in->buf, de->d_name, &st, 0, fill_dir_plus))
      break;
  }

  closedir(dp);
  return 0;
}

response *server::getattr(getattr_in *in) {
  int length = sizeof(getattr_out);
  auto buffer = new uint8_t[sizeof(uint64_t) + length];
  response *ptr = (response *)buffer;
  ptr->length = length;

  int res;

  res = lstat(in->path, &ptr->res.);
  if (res == -1)
    return nullptr;

  return ptr;
}*/
