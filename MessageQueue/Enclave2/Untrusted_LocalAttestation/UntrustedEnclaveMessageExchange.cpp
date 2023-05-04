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
extern sgx_enclave_id_t e1_enclave_id;

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
ATTESTATION_STATUS session_request_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg1_t* dh_msg1, uint32_t* session_id)
{
	uint32_t status = 0;
	sgx_status_t ret = SGX_SUCCESS;

    printf("[OCALL IPC] Generating msg1 and session_id for Enclave1\n");
    // for session_id
    printf("[OCALL IPC] Passing SessionID to shared memory for Enclave1\n");
	send_message(MESSAGE_QUEUE_NAME_RESPONSE, (const char*) session_id);

    // for msg1
    printf("[OCALL IPC] Passing message1 to shared memory for Enclave1\n");
	send_message(MESSAGE_QUEUE_NAME_RESPONSE, (const char*) dh_msg1);

    // let enclave1 to receive msg1
    printf("[OCALL IPC] Waiting for Enclave1 to process SessionID and message1...\n");

	if (ret == SGX_SUCCESS)
		return (ATTESTATION_STATUS)status;
	else
	    return INVALID_SESSION;
}

//Makes an sgx_ecall to the destination enclave sends message2 from the source enclave and gets message 3 from the destination enclave
ATTESTATION_STATUS exchange_report_ocall(sgx_enclave_id_t src_enclave_id, sgx_enclave_id_t dest_enclave_id, sgx_dh_msg2_t *dh_msg2, sgx_dh_msg3_t *dh_msg3, uint32_t session_id)
{
	uint32_t status = 0;
	sgx_status_t ret = SGX_SUCCESS;

    if (dh_msg3 == NULL)
    {
        // get msg2 from Enclave1
        printf("[OCALL IPC] Message2 should be ready\n");
		receive_message(MESSAGE_QUEUE_NAME_REQUEST, (char*) dh_msg2, sizeof(sgx_dh_msg2_t));
    }

    // ret = Enclave1_exchange_report(src_enclave_id, &status, 0, dh_msg2, dh_msg3, session_id);

    else
    {
        // pass msg3 to shm for Enclave
        printf("[OCALL IPC] Passing message3 to shared memory for Enclave1\n");
		send_message(MESSAGE_QUEUE_NAME_RESPONSE,  (char*) dh_msg3);
        // wait for Enclave1 to process msg3
        printf("[OCALL IPC] Waiting for Enclave1 to process message3...\n");
        // sleep(5);
    }

	if (ret == SGX_SUCCESS)
		return (ATTESTATION_STATUS)status;
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
