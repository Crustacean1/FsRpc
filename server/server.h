#ifndef SERVER_H
#define SERVER_H

#include "stream.h"
#include <cstdint>
#include <fcntl.h>
#include <functional>

//
std::vector<std::vector<char>> readdir(std::vector<char> path);
struct stat getattr(std::vector<char> path);
//
std::vector<uint8_t> execute(const std::vector<uint8_t> &);

template <typename R, typename... Args>
std::function<std::vector<uint8_t>(std::vector<uint8_t>)>
serialize_function(R (*func)(Args...)) {
  return [&func](const std::vector<uint8_t> &data) {
    auto args = deserialize<Args...>(data);
    auto result = std::apply(getattr, args);
    return serialize(result);
  };
}

int open(std::vector<char> filename, std::vector<char> mode);
std::vector<uint8_t> read(int file, int count);
int write(int file, std::vector<uint8_t> data);
int lseek(int file, int offset, int whence);
int chmod(std::vector<char> pathname, int mode);
int unlink(std::vector<char> pathname);
int rename(std::vector<char> oldpath, std::vector<char> newpath);

const std::function<std::vector<uint8_t>(const std::vector<uint8_t> &)>
    functions[] = {
        serialize_function(readdir),
        serialize_function(getattr),
};

#endif /*SERVER_H*/
