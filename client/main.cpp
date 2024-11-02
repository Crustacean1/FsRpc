#include "../common/stream.h"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

int main(int argc, char **argv) {
  // auto udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  addrinfo hints;
  addrinfo *results;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  auto addrinfo = getaddrinfo(argv[1], argv[2], &hints, &results);
  int sfd;
  for (auto rp = results; rp != nullptr; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }
    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }
    close(sfd);
  }
  freeaddrinfo(results);

  stream_context ctx(1);
  init(&ctx, sfd);

  std::thread thr([&ctx]() {
    std::cout << "Threading a needle" << std::endl;
    char wrt_buff[56] = "Jan Paweł Drugi Gwałcił Małe Dzieci o Godzinie 2137";
    std::cout << "Starting writing" << std::endl;
    write(&ctx, wrt_buff, 56);
    std::cout << "Done writing" << std::endl;
    char response[100];
    read(&ctx, response, 10);
    std::cout << "Response:" << std::endl;
    std::cout.write(response, 56);
  });

  auto epollfd = epoll_create(1);
  epoll_event epoll_ev;
  epoll_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  epoll_ev.data.fd = sfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &epoll_ev);

  constexpr size_t EVENT_COUNT = 10;
  constexpr size_t EPOLL_INTERVAL = 1000;
  epoll_event events[EVENT_COUNT];

  sockaddr_storage peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);

  size_t BUF_SIZE = 4096;
  char buf[BUF_SIZE];

  std::cout << "Starting event loop" << std::endl;
  while (true) {
    auto event_count = epoll_wait(epollfd, events, EVENT_COUNT, -1);
    std::cout << "Event transpired: " << event_count << std::endl;
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == sfd) {
        if (events[i].events & EPOLLIN) {
          std::cout << "message on client" << std::endl;
          int j = recvfrom(sfd, buf, BUF_SIZE, MSG_DONTWAIT,
                           (sockaddr *)&peer_addr, &peer_addrlen);
          memcpy(&ctx.peer_addr, &peer_addr, sizeof(sockaddr));
          ctx.peer_addrlen = peer_addrlen;
          receive_message(&ctx, (stream_message *)buf);
        } else {
          std::cout << "Socket disconnected" << std::endl;
          break;
        }
      } else {
        std::cout << "Utmost peculiarity" << std::endl;
      }
    }
  }
  thr.join();
  close(sfd);
  close(epollfd);
}
