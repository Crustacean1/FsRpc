#include "../common/file_server.h"
#include "../common/stream.h"
#include "server.h"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/syslog.h>
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

void process(stream_context *context) {
  int64_t len;
  std::vector<uint8_t> buffer;
  std::cout << "Rpc server started for: " << context->peer_addr.sa_data[0]
            << std::endl;
  while (true) {
    read(context, &len, sizeof(int64_t));
    if (buffer.size() < len) {
      buffer.resize(len);
    }

    syslog(LOG_INFO, "Reading: %lu", len);
    read(context, buffer.data(), len);
    syslog(LOG_INFO, "Executing");
    auto response = execute(buffer);
    write(context, response.data(), response.size());
  }
}

int main(int argc, char **argv) {
  // auto udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  addrinfo hints;
  addrinfo *results;
  openlog("rpc-server", LOG_PID | LOG_CONS, LOG_USER);

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

  int sfd;
  auto rp = results;
  for (; rp != nullptr; rp = rp->ai_next) {
    std::cout << "Found address!" << std::endl;
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }
    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
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
  sockaddr_in peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = handler;
  sigaction(2, &action, nullptr);

  auto epollfd = epoll_create(1);
  epoll_event epoll_ev;
  epoll_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  epoll_ev.data.fd = sfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &epoll_ev);

  constexpr size_t EVENT_COUNT = 10;
  constexpr size_t EPOLL_INTERVAL = 1000;
  epoll_event events[EVENT_COUNT];

  std::vector<std::thread> threads;
  std::vector<stream_context> contexts;

  while (should_run) {
    auto event_count = epoll_wait(epollfd, events, EVENT_COUNT, -1);
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == sfd) {
        if (events[i].events & EPOLLIN) {
          std::cout << "You got a mail!" << std::endl;
          int j = recvfrom(sfd, buf, BUF_SIZE, MSG_DONTWAIT,
                           (sockaddr *)&peer_addr, &peer_addrlen);

          int client_addr = peer_addr.sin_port;
          client_addr <<= 16;
          client_addr += peer_addr.sin_addr.s_addr;

          if (!clients.contains(client_addr)) {
            clients.emplace(client_addr, 1);
            init(&clients.at(client_addr), sfd);
            memcpy(&clients.at(client_addr).peer_addr, &peer_addr,
                   sizeof(sockaddr));
            clients.at(client_addr).peer_addrlen = peer_addrlen;
            stream_context *client = &clients.at(client_addr);
            threads.emplace_back([&client]() { process(client); });
          }

          stream_context *client = &clients.at(client_addr);
          stream_message *ptr = (stream_message *)buf;

          if (ptr->seq < 0) {
            on_ack(client, -((stream_message *)buf)->seq);
          } else {
            if (ptr->len <= BUFFER_SIZE) {
              on_msg(client, ptr->seq, ptr->len, (uint8_t *)ptr->data);
            } else {
              std::cerr << "Invalid packet length: " << ptr->len << std::endl;
            }
          }
        } else {
          std::cout << "Socket disconnected" << std::endl;
          break;
        }
      } else {
        std::cout << "Utmost peculiarity" << std::endl;
      }
    }
  }

  close(epollfd);
  close(sfd);
}
