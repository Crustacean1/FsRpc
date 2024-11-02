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

  for (int i = 0; i < 16; i++) {
    context->message_buffer[i].len = -1;
  }
  schedule_clock(context, 1000);
}

void read_buffer(stream_context *context) {
  int j;
  int length;
  for (; context->message_buffer[context->pending_window].len != -1;
       context->pending_window++) {
    std::cout << "Reading: " << context->pending_window << std::endl;
    int message_length =
        context->message_buffer[j].len - context->pending_offset;
    length = std::min(context->read_len - context->read_pos, message_length);
    memcpy((uint8_t *)context->read_buf + context->read_pos,
           context->message_buffer[j].data + context->pending_offset,
           message_length);
    context->read_pos += context->message_buffer[j].len;
    if (length == message_length) {
      context->message_buffer[j].len = -1;
    }
    if (context->read_pos == context->read_len) {
      if (context->message_buffer[j].len != -1) {
        context->pending_window = j;
        context->pending_offset += length;
      }
      context->expected_seq =
          context->message_buffer[context->pending_window].seq + 1;
      context->on_read();
      return;
    }
    context->pending_offset = 0;
  }
  context->expected_seq =
      context->message_buffer[context->pending_window].seq + 1;
}

void read(stream_context *context, void *stream, size_t length) {
  std::promise<void> prom;
  {
    const std::lock_guard lck(context->ctx_mutex);
    context->read_buf = stream;
    context->read_len = length;
    context->read_seq = context->expected_seq;
    context->on_read = [&prom]() { prom.set_value(); };
    read_buffer(context);
  }
  prom.get_future().get();
}

void write(stream_context *context, void *stream, size_t length) {
  std::promise<void> prom;
  {
    const std::lock_guard lck(context->ctx_mutex);
    context->write_buf = stream;
    context->write_len = length;
    context->write_seq = context->next_seq;
    context->on_write = [&prom]() { prom.set_value(); };
  }
  std::cout << "Waiting starts, lock released" << std::endl;
  prom.get_future().get();
}

void receive_message(stream_context *context, stream_message *message) {
  const std::lock_guard lck(context->ctx_mutex);
  if (message->seq < 0) {
    // Acknowledgement
    context->next_seq = std::max(context->next_seq, 1 - message->seq);
    if (context->write_len != 0 &&
        (context->next_seq - context->write_seq) * 1000 >= context->write_len) {
      context->write_len = 0;
      context->write_seq = 0;
      context->write_buf = nullptr;
      context->on_write();
      // Write completed
    }
    return;
  }

  if (message->seq >= context->expected_seq &&
      message->seq < context->expected_seq + 16) {
    std::cout << "Recevied message" << std::endl;
    auto offset = message->seq - context->expected_seq;
    if (context->message_buffer[offset].len == -1) {
      memcpy(context->message_buffer + offset, message,
             message->len + 3 * sizeof(int32_t));
      if (offset == 0) {
        read_buffer(context);
        ack_message ack;
        ack.seq = -(context->expected_seq - 1);
        sendto(context->sockfd, &ack, sizeof(ack), 0, &context->peer_addr,
               context->peer_addrlen);
      }
    } else {
      // Duplicate - resend acknowledgement
      ack_message ack;
      ack.seq = -(context->expected_seq);
      if (sendto(context->sockfd, &ack, sizeof(ack), 0, &context->peer_addr,
                 context->peer_addrlen) == -1) {
        std::cerr << "Failed to send ack" << std::endl;
        exit(1);
      }
    }
  }
}

void timer_elapsed(sigval_t si) {
  stream_context *context = (stream_context *)si.sival_ptr;
  {
    const std::lock_guard lck(context->ctx_mutex);
    stream_message msg;
    int seq = context->next_seq;
    while (seq < context->next_seq + 16 &&
           seq < context->write_seq + (context->write_len + 999) / 1000) {
      auto len = std::min(
          context->write_len - (seq - context->write_seq) * 1000, 1000);
      memcpy(msg.data,
             (uint8_t *)context->write_buf + (seq - context->write_seq) * 1000,
             len);
      msg.seq = seq++;
      send(context->sockfd, &msg, sizeof(int32_t) * 2 + len, 0);
    }
  }
}

void schedule_clock(stream_context *context, int time) {
  itimerspec its;
  its.it_value.tv_sec = 1;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = 1;
  its.it_interval.tv_nsec = 0;

  if (timer_settime(context->timer, 0, &its, nullptr) == -1) {
    std::cerr << "Failed to set stream timer" << std::endl;
    exit(-1);
  }
}
