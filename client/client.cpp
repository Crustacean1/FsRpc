#include "./client.h"
#include <cstring>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/syslog.h>
#include <thread>

std::thread *network_thread = nullptr;

int connect(stream_context *ctx) {
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

  memcpy(&ctx->peer_addr, results->ai_addr, sizeof(sockaddr));
  ctx->peer_addrlen = results->ai_addrlen;

  return sfd;
}

void event_loop(stream_context *ctx) {
  syslog(LOG_INFO, "Starting event loop: %lu", (uint64_t)ctx);

  auto epollfd = epoll_create(1);
  if (epollfd < 0) {
    syslog(LOG_ERR, "Failed to create monitor");
  }
  epoll_event epoll_ev;
  epoll_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  epoll_ev.data.fd = ctx->sockfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, ctx->sockfd, &epoll_ev);

  constexpr size_t EVENT_COUNT = 10;
  constexpr size_t EPOLL_INTERVAL = 1000;
  epoll_event events[EVENT_COUNT];

  sockaddr peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);

  size_t BUF_SIZE = 4096;
  char buf[BUF_SIZE];

  while (true) {
    auto event_count = epoll_wait(epollfd, events, EVENT_COUNT, -1);
    for (int i = 0; i < event_count; i++) {
      if (events[i].data.fd == ctx->sockfd) {
        if (events[i].events & EPOLLIN) {
          syslog(LOG_INFO, "Received packet");
          int j = recvfrom(ctx->sockfd, buf, BUF_SIZE, MSG_DONTWAIT,
                           (sockaddr *)&peer_addr, &peer_addrlen);
          stream_message *ptr = (stream_message *)buf;
          if (ptr->seq < 0) {
            on_ack(ctx, -((stream_message *)buf)->seq);
          } else {
            if (ptr->len < BUFFER_SIZE) {
              on_msg(ctx, ptr->seq, ptr->len, (uint8_t *)ptr->data);
            } else {
              syslog(LOG_ERR, "Invalid packet length: %d", ptr->len);
            }
          }
        } else {
          break;
        }
      } else {
        syslog(LOG_INFO, "Unexpected event");
        break;
      }
    }
  }
  syslog(LOG_ERR, "Loop exited");
  close(ctx->sockfd);
  close(epollfd);
}

void *client::init(fuse_conn_info *, fuse_config *) {
  stream_context *str_ctx = new stream_context();

  syslog(LOG_INFO, "initializing context: %lu", (uint64_t)str_ctx);
  auto sfd = connect(str_ctx);
  ::init(str_ctx, sfd);
  syslog(LOG_INFO, "post init context: %lu", (uint64_t)str_ctx);
  auto context = new client_context{
      str_ctx, std::thread([str_ctx]() { ::event_loop(str_ctx); })};
  return context;
}

void client::destroy(void *ctx) {
  syslog(LOG_INFO, "Cleaning up context");
  client_context *context = (client_context *)ctx;
  context->thr.join();
  delete context->context;
  delete context;
}

int client::open(const char *filename, fuse_file_info *) {
  // Dummy operation
  return 0;
}

int client::getattr(const char *filename, struct stat *st, fuse_file_info *fi) {
  syslog(LOG_INFO, "getattr");
  std::vector<char> filename_vec(filename, filename + strlen(filename) + 1);
  auto response = execute<struct stat>(
      ((client_context *)fuse_get_context()->private_data)->context, 1,
      filename_vec);
  memcpy(st, &response, sizeof(struct stat));
  return 0;
}
int client::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                    off_t offset, struct fuse_file_info *fi,
                    enum fuse_readdir_flags flags) {
  syslog(LOG_INFO, "readdir");
  std::vector<char> filename_vec(path, path + strlen(path) + 1);
  auto response = execute<std::vector<std::vector<char>>>(
      ((client_context *)fuse_get_context()->private_data)->context, 2,
      filename_vec);
  for (const auto &directory : response) {
    filler(buf, directory.data(), nullptr, 0, (fuse_fill_dir_flags)0);
  }
  return 0;
}
int client::read(const char *, char *, size_t, off_t, fuse_file_info *) {
  return 0;
}
int client::write(const char *, const char *, size_t, off_t, fuse_file_info *) {
  return 0;
}
