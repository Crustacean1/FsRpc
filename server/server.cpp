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
  syslog(LOG_INFO, "Executing function: %lu size: %lu", fn, data.size());
  std::vector<uint8_t> fn_data(data.begin() + sizeof(uint64_t), data.end() - 1);
  syslog(LOG_INFO, "Executing function: %lu", fn);
  auto response = functions[fn](fn_data);
  syslog(LOG_INFO, "Function executed");
  // Dirty hack
  std::move(fn_data);
  return response;
}

struct stat getattr(std::vector<char> path) {
  syslog(LOG_INFO, "Getting dem attributes");
  return stat{};
}
