#ifndef STREAM_H
#define STREAM_H

#include <cstdint>
#include <functional>
#include <mutex>
#include <signal.h>
#include <sys/socket.h>

constexpr int BUFFER_SIZE = 1000;
constexpr int WINDOW_LENGTH = 16;

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
  int32_t next_seq = 1;
  int32_t expected_seq = 1;
  stream_message message_buffer[WINDOW_LENGTH];
  int32_t pending_window = 0;
  int32_t pending_offset = 0;

  sockaddr peer_addr;
  socklen_t peer_addrlen = sizeof(sockaddr_storage);

  void *read_buf = nullptr;
  int32_t read_seq = 0;
  int32_t read_len = 0;
  int32_t read_pos = 0;
  std::function<void()> on_read;

  void *write_buf = nullptr;
  int32_t write_seq = 0;
  int32_t write_len = 0;
  std::function<void()> on_write;

  stream_context(int i) {}
  stream_context(stream_context &&a) = delete;
};

void write(stream_context *context, void *stream, size_t length);
void read(stream_context *context, void *stream, size_t length);
void timer_elapsed(sigval_t);
void init(stream_context *context, int sfd);
void receive_message(stream_context *context, stream_message *message);

#endif /*STREAM_H*/
