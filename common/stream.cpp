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

  for (int i = 0; i < WINDOW_LENGTH; i++) {
    context->message_buffer[i].len = -1;
  }
  schedule_clock(context, 0);
}

bool read_buffer(stream_context *context) {
  std::cout << "Starting: " << context->pending_window << "\t"
            << (uint64_t)context << std::endl;
  if (context->read_buf == nullptr) {
    std::cout << "Invalid buffer" << std::endl;
    return false;
  }
  int length;
  for (; context->message_buffer[context->pending_window].len != -1 &&
         context->read_pos < context->read_len;
       context->pending_window = (context->pending_window + 1) % WINDOW_LENGTH,
       context->message_buffer[context->pending_window].len = -1) {
    int message_length = context->message_buffer[context->pending_window].len -
                         context->pending_offset;
    length = std::min(context->read_len - context->read_pos, message_length);
    memcpy((uint8_t *)context->read_buf + context->read_pos,
           context->message_buffer[context->pending_window].data +
               context->pending_offset,
           length);
    context->read_pos += length;
    context->pending_offset =
        (context->pending_offset + length) % message_length;
  }
  if (context->read_pos == context->read_len) {
    context->read_buf = nullptr;
    context->on_read();
    std::cout << "New pending_window: " << context->pending_window << "\t"
              << (uint64_t)context << std::endl;
    return true;
  } else {
    std::cout << "New pending_window: " << context->pending_window << std::endl;
    return false;
  }
}

void read(stream_context *context, void *stream, size_t length) {
  std::promise<void> prom;
  {
    const std::lock_guard lck(context->ctx_mutex);
    context->read_buf = stream;
    context->read_len = length;
    context->read_seq = context->expected_seq;
    context->read_pos = 0;
    context->on_read = [&prom]() { prom.set_value(); };
    std::cout << "Starting read" << std::endl;
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

void acknowledge(stream_context *context) {
  ack_message ack;
  ack.seq = -(context->expected_seq);
  std::cout << "Sending ack: " << -ack.seq << std::endl;
  if (sendto(context->sockfd, &ack, sizeof(ack), 0, &context->peer_addr,
             context->peer_addrlen) == -1) {
    std::cerr << "Failed to send ack" << std::endl;
    exit(1);
  }
}

void receive_message(stream_context *context, stream_message *message) {
  const std::lock_guard lck(context->ctx_mutex);
  if (message->seq < 0) {
    std::cout << "Ack received: " << -message->seq << std::endl;
    // Acknowledgement
    context->next_seq = std::max(context->next_seq, 1 - message->seq);
    if (context->write_len != 0 &&
        (context->next_seq - context->write_seq) * BUFFER_SIZE >=
            context->write_len) {
      context->write_len = 0;
      context->write_seq = 0;
      context->write_buf = nullptr;
      context->on_write();
      // Write completed
    }
    return;
  }

  if (message->seq >= context->expected_seq &&
      message->seq < context->expected_seq + WINDOW_LENGTH) {
    auto offset = (message->seq + WINDOW_LENGTH - 1) % WINDOW_LENGTH;
    std::cout << "Received message: " << message->seq << "\t" << message->len
              << "\t" << offset << std::endl;
    if (context->message_buffer[offset].len == -1) {
      memcpy(context->message_buffer + offset, message,
             message->len + 2 * sizeof(int32_t));
	  context->expected_seq = std::max(context->expected_seq, )
      read_buffer(context);
    }
  }
  acknowledge(context);
}

void timer_elapsed(sigval_t si) {
  stream_context *context = (stream_context *)si.sival_ptr;
  {
    const std::lock_guard lck(context->ctx_mutex);
    stream_message msg;
    int seq = context->next_seq;
    while (seq < context->next_seq + WINDOW_LENGTH &&
           seq < context->write_seq +
                     (context->write_len + (BUFFER_SIZE - 1)) / BUFFER_SIZE) {
      auto len = std::min(context->write_len -
                              (seq - context->write_seq) * BUFFER_SIZE,
                          BUFFER_SIZE);
      memcpy(msg.data,
             (uint8_t *)context->write_buf +
                 (seq - context->write_seq) * BUFFER_SIZE,
             len);
      msg.seq = seq++;
      msg.len = len;
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
