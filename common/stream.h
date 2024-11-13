#ifndef STREAM_H
#define STREAM_H

#include <ranges>
#include <sys/syslog.h>
#define FUSE_USE_VERSION 31

#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <signal.h>
#include <sys/socket.h>

constexpr int BUFFER_SIZE = 10;
constexpr int WINDOW_LENGTH = 3;

template <typename T> size_t size(const T &t) { return sizeof(T); }

template <typename T> size_t size(const std::vector<T> &t) {
  return sizeof(T) * t.size();
}

template <typename T> void write_self(const T &t, uint8_t **buffer) {
  std::memcpy(*buffer, &t, sizeof(T));
  *buffer += sizeof(T);
}

template <typename R, template <typename> typename T>
void write_self(const T<R> &t, uint8_t **buffer) {
  *((uint64_t *)*buffer) = t.size();
  *buffer += sizeof(uint64_t);
  for (const auto &a : t) {
    write_self(a, buffer);
  }
}

template <typename T> void read_self(uint8_t **buffer, T &t) {
  T *ptr = (T *)*buffer;
  *buffer = (uint8_t *)(++ptr);
  t = *ptr;
}

template <typename R, template <typename> typename T>
void read_self(uint8_t **buffer, T<R> &container) {
  uint64_t *len = (uint64_t *)*buffer;
  uint8_t *ptr = *buffer + sizeof(uint64_t);
  for (int i = 0; i < *len; i++) {
    R r;
    read_self(&ptr, r);
    container.push_back(r);
  }
  *buffer = ptr;
}

template <typename... Args> std::vector<uint8_t> serialize(Args... args) {
  int data_size = (size(args) + ...);
  int buffer_size = data_size;
  std::vector<uint8_t> buffer(buffer_size + sizeof(uint64_t));
  uint8_t *ptr = buffer.data();
  *((uint64_t *)ptr) = buffer_size;
  ptr += sizeof(uint64_t);
  (write_self(args, &ptr), ...);
  return buffer;
}

template <typename... Args>
std::tuple<Args...> deserialize(const std::vector<uint8_t> &buffer) {
  uint8_t *ptr = (uint8_t *)buffer.data();
  std::tuple<Args...> tpl;

  std::apply([&ptr](auto &...args) { (read_self(&ptr, args), ...); }, tpl);
  return tpl;
}

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
  timer_t timer = nullptr;
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

  stream_context() {}
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

#endif /*STREAM_H*/
