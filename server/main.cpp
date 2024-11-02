#include "../common/stream.h"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

bool should_run = true;
void handler(int sig) {
  std::cout << "Terminating" << std::endl;
  should_run = false;
}

std::unordered_map<uint64_t, stream_context> clients;

void echo(stream_context *context) {
  auto BUF_LEN = 512;
  uint8_t buf[BUF_LEN];

  while (true) {
    std::cout << "Waiting for message" << std::endl;
    read(context, buf, 10);
    std::cout << "Reading response:" << std::endl;
	std::cout<<"\t";
    std::cout.write((char *)buf, 10);
    std::cout << "\nMessage END" << std::endl;
  }
}

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

  auto result = getaddrinfo(nullptr, "12345", &hints, &results);
  if (result != 0) {
    std::cerr << "Failed to find bindable address" << std::endl;
    return EXIT_FAILURE;
  }

  int sock;
  auto rp = results;
  for (; rp != nullptr; rp = rp->ai_next) {
    std::cout << "Found address!" << std::endl;
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) {
      continue;
    }
    if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;
    }
  }
  freeaddrinfo(results);
  if (rp == nullptr) {
    std::cerr << "Failed to bind to address" << std::endl;
    return EXIT_FAILURE;
  }
  size_t BUF_SIZE = 4096;
  char buf[BUF_SIZE];
  sockaddr peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = handler;
  sigaction(2, &action, nullptr);

  auto epollfd = epoll_create(1);
  epoll_event epoll_ev;
  epoll_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  epoll_ev.data.fd = sock;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &epoll_ev);

  constexpr size_t EVENT_COUNT = 10;
  constexpr size_t EPOLL_INTERVAL = 1000;
  epoll_event events[EVENT_COUNT];

  std::vector<std::thread> threads;
  std::vector<stream_context> contexts;

  while (should_run) {
    auto event_count = epoll_wait(epollfd, events, EVENT_COUNT, -1);
    for (int i = 0; i < event_count; i++) {
      std::cout << events[i].data.fd << "\t" << sock << std::endl;
      if (events[i].data.fd == sock) {
        if (events[i].events & EPOLLIN) {
          int j = recvfrom(sock, buf, BUF_SIZE, MSG_DONTWAIT,
                           (sockaddr *)&peer_addr, &peer_addrlen);
          if (clients.contains(1)) {
            receive_message(&clients.at(1), (stream_message *)buf);
          } else {
            clients.emplace(1, 1);

            auto client = &clients.at(1);
            init(client, sock);
            std::cout << "Peer addr: " << peer_addrlen << std::endl;
            memcpy(&client->peer_addr, &peer_addr, sizeof(sockaddr));
            client->peer_addrlen = peer_addrlen;
            threads.emplace_back([client]() { echo(client); });
            receive_message(client, (stream_message *)buf);
          }

        } else {
          std::cout << "Socket disconnected" << std::endl;
        }
      } else {
        std::cout << "Utmost peculiarity" << std::endl;
      }
    }
  }

  close(epollfd);
  close(sock);
}
