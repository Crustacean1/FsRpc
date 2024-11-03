#ifndef STREAM_H
#define STREAM_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <signal.h>
#include <sys/socket.h>

constexpr int BUFFER_SIZE = 10;
constexpr int WINDOW_LENGTH = 3;

struct stream_message {
  int32_t seq;
  int32_t len = 0;
  int8_t data[BUFFER_SIZE];
};

struct ack_message {
  int32_t seq;
  int32_t len = 0;
};

class stream_context {
public:
  std::mutex ctx_mutex;
  void *timer = nullptr;
  int32_t sockfd;
  int32_t remote_seq = 0;
  int32_t seq = 1;
  int32_t retries = 0;

  sockaddr peer_addr;
  socklen_t peer_addrlen = sizeof(sockaddr_storage);

  uint8_t stream_buf[BUFFER_SIZE];
  int32_t stream_buf_len = 0;
  void *read_buf = nullptr;
  int32_t read_seq = 0;
  int32_t read_len = 0;
  int32_t read_pos = 0;
  std::function<void()> on_read;

  void *write_buf = nullptr;
  int32_t write_seq = 0;
  int32_t write_len = 0;
  int32_t write_pos = 0;
  std::function<void()> on_write;

  stream_context(int i) {}
  stream_context(stream_context &&a) = delete;
  stream_context(const stream_context &a) = delete;
};

void write(stream_context *context, void *stream, int length);
void read(stream_context *context, void *stream, size_t length);
void timer_elapsed(sigval_t);
void init(stream_context *context, int sfd);
void receive_message(stream_context *context, stream_message *message);

void on_ack(stream_context *, int seq);
void on_msg(stream_context *, int seq, int len, uint8_t *buf);
void on_timeout(sigval_t);

#endif /*STREAM_H*/
