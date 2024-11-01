#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // auto udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  addrinfo hints;
  addrinfo *results;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  auto addrinfo = getaddrinfo(nullptr, "", &hints, &results);
  for (auto rp = results; rp != nullptr; rp = rp->ai_next) {
    for (int i = 0; i < 14; i++) {
      std::cout << (int)rp->ai_addr->sa_data[i] << "\t";
    }
    std::cout << rp->ai_addrlen << std::endl;
  }
}
