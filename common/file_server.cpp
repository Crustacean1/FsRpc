#include "file_server.h"
#include "stream.h"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

response *execute(command *command) {
  response *ptr;
  switch (command->function) {
  case 1:
    ptr = open_fs((open_in *)command->data);
    break;
  case 2:
    ptr = read_fs((read_in *)command->data);
    break;
  case 3:
    ptr = write_fs((write_in *)command->data);
    break;
  case 4:
    ptr = lseek_fs((lseek_in *)command->data);
    break;
  case 5:
    ptr = chmod_fs((chmod_in *)command->data);
    break;
  case 6:
    ptr = unlink_fs((unlink_in *)command->data);
    break;
  case 7:
    ptr = rename_fs((rename_in *)command->data);
    break;
  default:
    std::cerr << "Invalid function specification: " << command->function
              << std::endl;
    return nullptr;
  }
  return ptr;
}

response *open_fs(open_in *in) {
  int desc = open(in->pathname + (uint64_t)&in, in->mode);
  auto buffer = new uint8_t[sizeof(open_out) + sizeof(uint64_t)];
  response *ptr = (response *)buffer;
  ptr->res.open.descriptor = desc;
  ptr->length = sizeof(open_out);
  return ptr;
}

response *read_fs(read_in *in) {
  auto buffer = new uint8_t[sizeof(uint64_t) + sizeof(read_out) + in->count];
  response *ptr = (response *)buffer;
  auto count = read(in->descriptor, (&ptr->res.read + 1), in->count);
  ptr->res.read.count = count;
  ptr->length = count + sizeof(read_out);
  return ptr;
}

response *write_fs(write_in *in) {
  auto count =
      write(in->descriptor, (uint8_t *)in->data + (uint64_t)&in, in->count);
  auto buffer = new uint8_t[sizeof(uint64_t) + sizeof(write_out)];
  response *ptr = (response *)buffer;
  ptr->res.write.count = count;
  ptr->length = sizeof(write_out);
  return ptr;
}

response *lseek_fs(lseek_in *) { return nullptr; }
response *chmod_fs(chmod_in *) { return nullptr; }
response *unlink_fs(unlink_in *) { return nullptr; }
response *rename_fs(rename_in *) { return nullptr; }
