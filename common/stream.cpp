#include "stream.h"
#include <csignal>
#include <cstdint>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <sys/socket.h>

void schedule_clock(stream_context *context, int time);
void send_msg(stream_context *context);
void acknowledge(stream_context *context);

void init(stream_context *context, int sfd) {
  timer_t timerid;
  sigevent sev;
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = timer_elapsed;
  sev.sigev_value.sival_ptr = (void *)context;
  sev.sigev_notify_attributes = nullptr;

  if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
    std::cerr << "Failed to create stream timer" << std::endl;
    exit(-1);
  }
  context->timer = timerid;
  context->sockfd = sfd;
}

void read(stream_context *context, void *stream, size_t length) {
  std::promise<void> prom;
  {
    const std::lock_guard lck(context->ctx_mutex);
    context->read_buf = stream;
    context->read_len = length;
    context->read_pos = 0;
    context->on_read = [&prom]() { prom.set_value(); };

    if (context->stream_buf_len != 0) {
      int length = std::min(context->stream_buf_len,
                            context->read_len - context->read_pos);
      memcpy((uint8_t *)context->read_buf + context->read_pos,
             context->stream_buf, length);
      context->read_pos += length;

      if (length != context->stream_buf_len) {
        std::memmove(context->stream_buf, context->stream_buf + length,
                     context->stream_buf_len - length);
      }
      context->stream_buf_len = context->stream_buf_len - length;
    }
    if (context->read_pos == context->read_len) {
      context->read_buf = nullptr;
      acknowledge(context);
      return;
    }
  }
  prom.get_future().get();
}

void write(stream_context *context, void *stream, int length) {
  std::promise<void> prom;
  {
    const std::lock_guard lck(context->ctx_mutex);
    context->write_buf = stream;
    context->write_len = length;
    context->write_pos = 0;
    context->on_write = [&prom]() { prom.set_value(); };
    send_msg(context);
  }
  prom.get_future().get();
  return;
}

void send_msg(stream_context *context) {
  stream_message msg;
  msg.seq = context->seq;

  msg.len = std::min(BUFFER_SIZE, context->write_len - context->write_pos);
  memcpy(msg.data, (uint8_t *)context->write_buf + context->write_pos, msg.len);
  if (sendto(context->sockfd, (void *)&msg, msg.len + 2 * sizeof(int32_t), 0,
             &context->peer_addr, context->peer_addrlen) == -1) {
    std::cerr << "Failed to send msg" << std::endl;
  }
  schedule_clock(context, 2000);
}

void on_ack(stream_context *context, int seq) {
  const std::lock_guard lck(context->ctx_mutex);
  schedule_clock(context, 0);
  // std::cout << "Ack: " << seq << "\t" << context->seq << std::endl;
  if (seq == context->seq) {
    context->seq++;
    context->write_pos =
        std::min(context->write_pos + BUFFER_SIZE, context->write_len);
    if (context->write_pos == context->write_len) {
      context->write_buf = nullptr;
      context->on_write();
    } else {
      send_msg(context);
      schedule_clock(context, 2000);
    }
  }
  // std::cout << "On ack" << std::endl;
}

void acknowledge(stream_context *context) {
  ack_message ack;
  ack.seq = -(context->remote_seq);
  if (sendto(context->sockfd, &ack, sizeof(ack), 0, &context->peer_addr,
             context->peer_addrlen) == -1) {
    std::cerr << "Failed to send ack" << std::endl;
    exit(1);
  }
}

void on_msg(stream_context *context, int seq, int len, uint8_t *buf) {
  const std::lock_guard lck(context->ctx_mutex);
  if (context->read_buf != nullptr) {
    if (seq == context->remote_seq + 1) {
      int length = std::min(len, context->read_len - context->read_pos);
      memcpy((uint8_t *)context->read_buf + context->read_pos, buf, length);
      context->read_pos += length;
      if (length != len) {
        context->stream_buf_len = len - length;
        memcpy(context->stream_buf, buf + length, context->stream_buf_len + 3);
      }
      context->remote_seq = seq;
    }
    if (seq <= context->remote_seq) {
      acknowledge(context);
    }
    if (context->read_pos == context->read_len) {
      context->read_buf = nullptr;
      context->on_read();
    }
  }
}

void timer_elapsed(sigval_t si) {
  stream_context *context = (stream_context *)si.sival_ptr;
  const std::lock_guard lck(context->ctx_mutex);
  if (context->write_buf != nullptr) {
    //std::cout << "timeout: " << context->seq << "\t" << context->remote_seq
              //<< std::endl;
    send_msg(context);
  }
}

void schedule_clock(stream_context *context, int time) {
  itimerspec its;
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = time;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;

  if (timer_settime(context->timer, 0, &its, nullptr) == -1) {
    std::cerr << "Failed to set stream timer" << std::endl;
    exit(-1);
  }
}

void on_timeout(sigval_t) {}
