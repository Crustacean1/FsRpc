#ifndef FILE_H
#define FILE_H

#include "stream.h"
#include <cstdint>
#include <tuple>
#include <unistd.h>

struct data {
  int length;
  void *data;
};

struct open_in {
  const char *pathname;
  char mode;
};

struct open_out {
  int descriptor;
};

struct read_in {
  int descriptor;
  int count;
};

struct read_out {
  int count;
};

struct write_in {
  int descriptor;
  int count;
  void *data;
};

struct write_out {
  ssize_t count;
};

struct lseek_in {};
struct lseek_out {};
struct chmod_in {};
struct chmod_out {};
struct unlink_in {};
struct unlink_out {};
struct rename_in {};
struct rename_out {};

union result {
  open_out open;
  read_out read;
  write_out write;
  lseek_out lseek;
  chmod_out chmod;
  unlink_out unlink;
  rename_out rename;
};

struct response {
  int64_t length;
  result res;
};

response *execute(command *command);

response *open_fs(open_in *);
response *read_fs(read_in *);
response *write_fs(write_in *);
response *lseek_fs(lseek_in *);
response *chmod_fs(chmod_in *);
response *unlink_fs(unlink_in *);
response *rename_fs(rename_in *);

#endif /*FILE_H*/
