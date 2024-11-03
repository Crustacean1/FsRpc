#include <sys/syslog.h>
#define FUSE_USE_VERSION 31

#include "../common//file_client.h"
#include "../common/stream.h"
#include <fuse.h>
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

void start_event_loop(stream_context *ctx) {
  // auto udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  addrinfo hints;
  addrinfo *results;

  syslog(LOG_INFO, "Awake");
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  auto addrinfo = getaddrinfo("127.0.0.1", "12345", &hints, &results);
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

  syslog(LOG_INFO, "Address selected");
  if (sfd < 0) {
    syslog(LOG_ERR, "Failed to create socket");
  }

  init(ctx, sfd);
  memcpy(&ctx->peer_addr, results->ai_addr, sizeof(sockaddr));
  ctx->peer_addrlen = results->ai_addrlen;

  auto epollfd = epoll_create(1);
  if (epollfd < 0) {
    syslog(LOG_ERR, "Failed to create monitor");
  }
  epoll_event epoll_ev;
  epoll_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  epoll_ev.data.fd = sfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &epoll_ev);

  constexpr size_t EVENT_COUNT = 10;
  constexpr size_t EPOLL_INTERVAL = 1000;
  epoll_event events[EVENT_COUNT];

  sockaddr peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);

  size_t BUF_SIZE = 4096;
  char buf[BUF_SIZE];

  while (true) {
    syslog(LOG_INFO, "Starting loop: %d and %d", ctx->sockfd, sfd);
    auto event_count = epoll_wait(epollfd, events, EVENT_COUNT, -1);
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == sfd) {
        if (events[i].events & EPOLLIN) {
          syslog(LOG_INFO, "Received packet");
          int j = recvfrom(sfd, buf, BUF_SIZE, MSG_DONTWAIT,
                           (sockaddr *)&peer_addr, &peer_addrlen);
          stream_message *ptr = (stream_message *)buf;
          if (ptr->seq < 0) {
            on_ack(ctx, -((stream_message *)buf)->seq);
          } else {
            if (ptr->len < BUFFER_SIZE) {
              on_msg(ctx, ptr->seq, ptr->len, (uint8_t *)ptr->data);
            } else {
              std::cerr << "Invalid packet length: " << ptr->len << std::endl;
            }
          }
        } else {
          break;
        }
      } else {
        std::cout << "Utmost peculiarity" << std::endl;
      }
    }
  }
  syslog(LOG_ERR, "Loop exited");
  close(sfd);
  close(epollfd);
}

const fuse_operations xmp_oper = {.getattr = client::getattr,
                                  .unlink = nullptr,
                                  .chmod = nullptr,
                                  .open = client::open,
                                  .read = client::read,
                                  .write = client::write,
                                  .readdir = client::readdir,
                                  .lseek = nullptr};

int main(int argc, char **argv) {
  stream_context ctx(1);
  openlog("rpc-client", LOG_PID | LOG_CONS, LOG_USER);
  std::thread thr([&ctx]() { start_event_loop(&ctx); });
  client::init(&ctx);
  syslog(LOG_INFO, "pre result");
  int result = fuse_main(argc, argv, &xmp_oper, NULL);
  syslog(LOG_INFO, "pre join");
  thr.join();
  syslog(LOG_INFO, "thread joined");
  return result;
}
