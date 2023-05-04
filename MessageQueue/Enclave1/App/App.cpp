#include <stdio.h>
#include <map>
#include "../Enclave1/Enclave1_u.h"
#include "sgx_eid.h"
#include "sgx_urts.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <chrono>

#define UNUSED(val) (void)(val)
#define TCHAR   char
#define _TCHAR  char
#define _T(str) str
#define scanf_s scanf
#define _tmain  main

extern std::map<sgx_enclave_id_t, uint32_t>g_enclave_id_map;

sgx_enclave_id_t e1_enclave_id = 0;

#define ENCLAVE1_PATH "libenclave1.so"

void waitForKeyPress()
{
    char ch;
    int temp;
    printf("\n\nHit a key....\n");
    temp = scanf_s("%c", &ch);
}

uint32_t load_enclaves()
{
    uint32_t enclave_temp_no;
    int ret, launch_token_updated;
    sgx_launch_token_t launch_token;

    enclave_temp_no = 0;

    ret = sgx_create_enclave(ENCLAVE1_PATH, SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &e1_enclave_id, NULL);
    if (ret != SGX_SUCCESS) {
                return ret;
    }

    enclave_temp_no++;
    g_enclave_id_map.insert(std::pair<sgx_enclave_id_t, uint32_t>(e1_enclave_id, enclave_temp_no));

    return SGX_SUCCESS;
}

int _tmain(int argc, _TCHAR* argv[])
{
    uint32_t ret_status;
    sgx_status_t status;

    UNUSED(argc);
    UNUSED(argv);

    if(load_enclaves() != SGX_SUCCESS)
    {
        printf("\nLoad Enclave Failure");
    }

    printf("\nAvailable Enclaves");
    printf("\nEnclave1 - EnclaveID %" PRIx64 "\n", e1_enclave_id);
    
    auto start = std::chrono::high_resolution_clock::now();

    // shared memory
    key_t key = ftok("../..", 1);
    int shmid = shmget(key, 1024, 0666|IPC_CREAT);
    char *str = (char*)shmat(shmid, (void*)0, 0);
    printf("[TEST IPC] Sending to Enclave2: Hello from Enclave1\n");
    strncpy(str, "Hello from Enclave1\n", 20);
    shmdt(str);

    do
    {
        printf("[START] Testing create session between Enclave1 (Initiator) and Enclave2 (Responder)\n");
        status = Enclave1_test_create_session(e1_enclave_id, &ret_status, e1_enclave_id, 0);
        status = SGX_SUCCESS;
        if (status!=SGX_SUCCESS)
        {
            printf("[END] test_create_session Ecall failed: Error code is %x\n", status);
            break;
        }
        else
        {
            if(ret_status==0)
            {
                printf("[END] Secure Channel Establishment between Initiator (E1) and Responder (E2) Enclaves successful !!!\n");
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> execution_time = end - start;
                printf("Enclave execution time for Enclav1: %.3f ms\n", execution_time.count());
            }
            else
            {
                printf("[END] Session establishment and key exchange failure between Initiator (E1) and Responder (E2): Error code is %x\n", ret_status);
                break;
            }
        }

#pragma warning (push)
#pragma warning (disable : 4127)
    }while(0);
#pragma warning (pop)

    sgx_destroy_enclave(e1_enclave_id);

    waitForKeyPress();

    return 0;
}
