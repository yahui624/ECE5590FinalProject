#include "queue.h"
#include <cstring>
#include <iostream>

void open_message_queue(const char* queue_name, mqd_t* message_queue) {
  *message_queue = mq_open(queue_name, O_RDONLY);
  if (*message_queue == (mqd_t)-1) {
    std::cerr << "Failed to open message queue " << queue_name << std::endl;
  }
}

void send_message(const char* queue_name, const char* message) {
  mqd_t message_queue = mq_open(queue_name, O_WRONLY);
  if (message_queue == (mqd_t)-1) {
    std::cerr << "Failed to open message queue " << queue_name << std::endl;
    return;
  }

  if (mq_send(message_queue, message, strlen(message), 0) == -1) {
    std::cerr << "Failed to send message to queue " << queue_name << std::endl;
  }

  mq_close(message_queue);
}

void receive_message(const char* queue_name, char* buffer, size_t buffer_size) {
  mqd_t message_queue = mq_open(queue_name, O_RDONLY);
  if (message_queue == (mqd_t)-1) {
    std::cerr << "Failed to open message queue " << queue_name << std::endl;
    return;
  }

  ssize_t message_size = mq_receive(message_queue, buffer, buffer_size, nullptr);
  if (message_size == -1) {
    std::cerr << "Failed to receive message from queue " << queue_name << std::endl;
  }

  buffer[message_size] = '\0';
  mq_close(message_queue);
}
