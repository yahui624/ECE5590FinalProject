#include "sgx_eid.h"
#include "error_codes.h"
#include "datatypes.h"
#include "sgx_urts.h"
#include "UntrustedEnclaveMessageExchange.h"
#include "sgx_dh.h"
#include <map>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <cstring>

const char* MESSAGE_QUEUE_NAME_REQUEST = "/my_message_queue_request";
const char* MESSAGE_QUEUE_NAME_RESPONSE = "/my_message_queue_response";

std::map<sgx_enclave_id_t, uint32_t>g_enclave_id_map;


void send_message(const char* queue_name, const char* message) {
  mqd_t message_queue = mq_open(queue_name, O_WRONLY);
  if (message_queue == (mqd_t)-1) {
	throw MALLOC_ERROR;
  }

  if (mq_send(message_queue, message, strlen(message), 0) == -1) {
	throw ATTESTATION_SE_ERROR;
  }

  mq_close(message_queue);
}

void receive_message(const char* queue_name, char* buffer, size_t buffer_size) {
  mqd_t message_queue = mq_open(queue_name, O_RDONLY);
  if (message_queue == (mqd_t)-1) {
	throw MALLOC_ERROR;
  }

  ssize_t bytes_received = mq_receive(message_queue, buffer, buffer_size, nullptr);
  if (bytes_received == -1) {
	throw ATTESTATION_SE_ERROR;
  }

  buffer[bytes_received] = '\0';

  mq_close(message_queue);
}


//Makes an sgx_ecall to the destination enclave to get session id and message1
// ATTESTATION_STATUS session_request_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg1_t* dh_msg1, uint32_t* session_id)
// {
// 	uint32_t status = 0;
// 	sgx_status_t ret = SGX_SUCCESS;

//   printf("[OCALL IPC] Waiting for Enclave2 to generate SessionID and message1...\n");
//   send_message(MESSAGE_QUEUE_NAME_REQUEST, "generate_session_request"); // send message to Enclave2 to indicate that it should generate SessionID and message1

//   // wait for Enclave2 to fill msg1
//   printf("[OCALL IPC] Waiting for Enclave2 to generate SessionID and message1...\n");
//   // sleep(5);
//   char buffer[1024];
//   receive_message(MESSAGE_QUEUE_NAME_RESPONSE, buffer, sizeof(buffer));
//   if (strcmp(buffer, "session_request_generated") != 0) {
//     return INVALID_REQUEST_TYPE_ERROR;  
//   }

//   printf("[OCALL IPC] SessionID and message1 should be ready\n");

//   // for session id
//   printf("[OCALL IPC] Retriving SessionID from shared memory\n");
//   // key_t key_session_id = ftok("../..", 3);
//   // int shmid_session_id = shmget(key_session_id, sizeof(uint32_t), 0666|IPC_CREAT);
//   // uint32_t* tmp_session_id = (uint32_t*)shmat(shmid_session_id, (void*)0, 0);
//   // memcpy(session_id, tmp_session_id, sizeof(uint32_t));
//   // shmdt(tmp_session_id);
//   receive_message(MESSAGE_QUEUE_NAME_RESPONSE, session_id, sizeof(*session_id));


//   // for msg1
//   printf("[OCALL IPC] Retriving message1 from shared memory\n");
//   // key_t key_msg1 = ftok("../..", 2);
//   // int shmid_msg1 = shmget(key_msg1, sizeof(sgx_dh_msg1_t), 0666|IPC_CREAT);
//   // sgx_dh_msg1_t *tmp_msg1 = (sgx_dh_msg1_t*)shmat(shmid_msg1, (void*)0, 0);
//   // memcpy(dh_msg1, tmp_msg1, sizeof(sgx_dh_msg1_t));
//   // shmdt(tmp_msg1);
//   receive_message(MESSAGE_QUEUE_NAME_RESPONSE, dh_msg1, sizeof(*dh_msg1));

//   ret = SGX_SUCCESS;

// 	if (ret == SGX_SUCCESS)
// 		return SUCCESS;
// 	else
// 	    return INVALID_SESSION;
// }

ATTESTATION_STATUS session_request_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg1_t* dh_msg1, uint32_t* session_id)
{
    uint32_t status = 0;
    sgx_status_t ret = SGX_SUCCESS;

    // wait for Enclave2 to fill msg1
    printf("[OCALL IPC] Waiting for Enclave2 to generate SessionID and message1...\n");
    sleep(5);

    printf("[OCALL IPC] SessionID and message1 should be ready\n");

    // send message to Enclave2 to indicate that it should generate SessionID and message1
    send_message(MESSAGE_QUEUE_NAME_REQUEST, "generate_session_request");
    // wait for Enclave2 to send a response indicating that it has generated SessionID and message1
    char buffer[1024];
    receive_message(MESSAGE_QUEUE_NAME_RESPONSE, buffer, sizeof(buffer));
    if (strcmp(buffer, "session_request_generated") != 0) {
        return INVALID_REQUEST_TYPE_ERROR;
    }

    // receive the session_id from Enclave2
    receive_message(MESSAGE_QUEUE_NAME_RESPONSE, (char*)session_id, sizeof(uint32_t));
    // receive the dh_msg1 from Enclave2
    receive_message(MESSAGE_QUEUE_NAME_RESPONSE, (char*)dh_msg1, sizeof(sgx_dh_msg1_t));

    ret = SGX_SUCCESS;
    if (ret == SGX_SUCCESS)
        return SUCCESS;
    else
        return INVALID_SESSION;
}

// //Makes an sgx_ecall to the destination enclave sends message2 from the source enclave and gets message 3 from the destination enclave
// ATTESTATION_STATUS exchange_report_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg2_t *dh_msg2, sgx_dh_msg3_t *dh_msg3, uint32_t session_id)
// {
// 	uint32_t status = 0;
// 	sgx_status_t ret = SGX_SUCCESS;

//     // for msg2 (filled by Enclave1)
//     printf("[OCALL IPC] Passing message2 to shared memory for Enclave2\n");
//     key_t key_msg2 = ftok("../..", 4);
//     int shmid_msg2 = shmget(key_msg2, sizeof(sgx_dh_msg2_t), 0666|IPC_CREAT);
//     sgx_dh_msg2_t *tmp_msg2 = (sgx_dh_msg2_t*)shmat(shmid_msg2, (void*)0, 0);
//     memcpy(tmp_msg2, dh_msg2, sizeof(sgx_dh_msg2_t));
//     shmdt(tmp_msg2);

//     // wait for Enclave2 to process msg2
//     printf("[OCALL IPC] Waiting for Enclave2 to process message2 and generate message3...\n");
//     sleep(5);

//     // retrieve msg3 (filled by Enclave2)
//     printf("[OCALL IPC] Message3 should be ready\n");
//     printf("[OCALL IPC] Retrieving message3 from shared memory\n");
//     key_t key_msg3 = ftok("../..", 5);
//     int shmid_msg3 = shmget(key_msg3, sizeof(sgx_dh_msg3_t), 0666|IPC_CREAT);
//     sgx_dh_msg3_t *tmp_msg3 = (sgx_dh_msg3_t*)shmat(shmid_msg3, (void*)0, 0);
//     memcpy(dh_msg3, tmp_msg3, sizeof(sgx_dh_msg3_t));
//     shmdt(tmp_msg3);

//     ret = SGX_SUCCESS;
// 	if (ret == SGX_SUCCESS)
// 		return SUCCESS;
// 	else
// 	    return INVALID_SESSION;
// }


ATTESTATION_STATUS exchange_report_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg2_t *dh_msg2, sgx_dh_msg3_t *dh_msg3, uint32_t session_id)
{
	uint32_t status = 0;
	sgx_status_t ret = SGX_SUCCESS;

  // send message2 to Enclave2
  send_message(MESSAGE_QUEUE_NAME_REQUEST, (char*)dh_msg2);
  
  // wait for Enclave2 to process msg2
  printf("[OCALL IPC] Waiting for Enclave2 to process message2 and generate message3...\n");
  sleep(5);

  // receive message3 from Enclave2
  printf("[OCALL IPC] Message3 should be ready\n");
  printf("[OCALL IPC] Retrieving message3 from message queue\n");
  receive_message(MESSAGE_QUEUE_NAME_RESPONSE, (char*)dh_msg3, sizeof(sgx_dh_msg3_t));

  ret = SGX_SUCCESS;
	if (ret == SGX_SUCCESS)
		return SUCCESS;
	else
	    return INVALID_SESSION;

}

//Make an sgx_ecall to the destination enclave function that generates the actual response
ATTESTATION_STATUS send_request_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id,secure_message_t* req_message, size_t req_message_size, size_t max_payload_size, secure_message_t* resp_message, size_t resp_message_size)
{
	uint32_t status = 0;
    sgx_status_t ret = SGX_SUCCESS;
	uint32_t temp_enclave_no;

	std::map<sgx_enclave_id_t, uint32_t>::iterator it = g_enclave_id_map.find(dest_enclave_id);
    if(it != g_enclave_id_map.end())
	{
		temp_enclave_no = it->second;
	}
    else
	{
		return INVALID_SESSION;
	}

	switch(temp_enclave_no)
	{
		case 1:
			ret = Enclave1_generate_response(dest_enclave_id, &status, src_enclave_id, req_message, req_message_size, max_payload_size, resp_message, resp_message_size);
			break;
	}
	if (ret == SGX_SUCCESS)
		return (ATTESTATION_STATUS)status;
	else
	    return INVALID_SESSION;

}

//Make an sgx_ecall to the destination enclave to close the session
ATTESTATION_STATUS end_session_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id)
{
	uint32_t status = 0;
	sgx_status_t ret = SGX_SUCCESS;
	uint32_t temp_enclave_no;

	std::map<sgx_enclave_id_t, uint32_t>::iterator it = g_enclave_id_map.find(dest_enclave_id);
    if(it != g_enclave_id_map.end())
	{
		temp_enclave_no = it->second;
	}
    else
	{
		return INVALID_SESSION;
	}

	switch(temp_enclave_no)
	{
		case 1:
			ret = Enclave1_end_session(dest_enclave_id, &status, src_enclave_id);
			break;
	}
	if (ret == SGX_SUCCESS)
		return (ATTESTATION_STATUS)status;
	else
	    return INVALID_SESSION;

}

void ocall_print_string(const char *str)
{
    printf("%s", str);
}