#include "./server.h"
#include <stdexcept>
#include <sys/syslog.h>

std::vector<uint8_t> execute(const std::vector<uint8_t> &data) {
  auto fn = *((uint64_t *)data.data());
  if (fn > sizeof(functions)) {
    syslog(LOG_ERR, "Invalid function specification: %lu, %lu", fn,
           data.size());
    throw std::runtime_error("Invalid function specification");
  }
  std::vector<uint8_t> fn_data(data.begin() + sizeof(uint64_t), data.end() - 1);
  auto response = functions[fn](fn_data);
  return response;
}

struct stat getattr(std::vector<char> path) {
  syslog(LOG_INFO, "Getting dem attributes: %u %zu", path[1], path.size());
  return stat{};
}
