#ifndef QUEUE_H
#define QUEUE_H

#include <mqueue.h>

#define MESSAGE_QUEUE_NAME_REQUEST "/my_message_queue_request"
#define MESSAGE_QUEUE_NAME_RESPONSE "/my_message_queue_response"

void open_message_queue(const char* queue_name, mqd_t* message_queue);
void send_message(const char* queue_name, const char* message);
void receive_message(const char* queue_name, char* buffer, size_t buffer_size);

#endif
