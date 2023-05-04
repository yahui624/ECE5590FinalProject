#ifndef IPC_MESSAGE_H
#define IPC_MESSAGE_H

#include "sgx_key_exchange.h"

typedef struct {
    int type; // 1 for message1, 2 for message2
    union {
        sgx_ra_msg1_t msg1;
        sgx_ra_msg2_t msg2;
    };
} ipc_message_t;

#endif // IPC_MESSAGE_H
