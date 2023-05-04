#include <openenclave/host.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "switchless_sample_u.h"

double get_relative_time_in_microseconds()
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    return (double)current_time.tv_sec * 1000000 +
           (double)current_time.tv_nsec / 1000.0;
}

void host_increment_switchless(int* n) {
    // Switchless called by the enclave 
    // allows the calling thread to continue executing other tasks without waiting for the host function to complete
    *n = *n + 1;
}

void host_increment_regular(int* n) {
    // Regular called by the enclave
    // requires context-switching between the host and the enclave
    *n = *n + 1;
}

static bool check_simulate_opt(int* argc, const char* argv[])
{
    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "--simulate") == 0)
        {
            fprintf(stderr, "Running in simulation mode\n");
            memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
            (*argc)--;
            return true;
        }
    }
    return false;
}

int main(int argc, const char* argv[])
{
    oe_enclave_t* enclave = NULL;
    oe_result_t result;
    int ret = 1, m = 1000000, n = 1000000;
    int oldm = m;
    double switchless_microseconds = 0;
    double start, end;

    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH [--simulate]\n", argv[0]);
        return 1;
    }

    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    if (check_simulate_opt(&argc, argv))
    {
        flags |= OE_ENCLAVE_FLAG_SIMULATE;
    }
    oe_enclave_setting_context_switchless_t switchless_setting = {
        1,  // number of host worker threads
        1}; // number of enclave worker threads.
    oe_enclave_setting_t settings[] = {{ // initalize the enclave with structure address and setting type
        .setting_type = OE_ENCLAVE_SETTING_CONTEXT_SWITCHLESS,  
        .u.context_switchless_setting = &switchless_setting,
    }};

    if ((result = oe_create_switchless_sample_enclave( // pass the settingsp[]
             argv[1],
             OE_ENCLAVE_TYPE_SGX,
             flags,
             settings,
             OE_COUNTOF(settings),
             &enclave)) != OE_OK)
        fprintf(stderr, "oe_create_enclave(): result=%u", result);

    start = get_relative_time_in_microseconds();

    // Call into the enclave
    result = enclave_add_N_switchless(enclave, &m, n);

    end = get_relative_time_in_microseconds();

    if (result != OE_OK)
    {
        fprintf(stderr, "enclave_add_N_switchless(): result=%u", result);
        goto done;
    }

    fprintf(
        stderr,
        "%d host_increment_switchless() calls: %d + %d = %d. Time spent: "
        "%d ms\n",
        n,
        oldm,
        n,
        m,
        (int)(end - start) / 1000);

    start = get_relative_time_in_microseconds();

    // Call into the enclave
    m = oldm;
    result = enclave_add_N_regular(enclave, &m, n);

    end = get_relative_time_in_microseconds();

    if (result != OE_OK)
    {
        fprintf(stderr, "enclave_add_N_regular(): result=%u", result);
        goto done;
    }

    fprintf(
        stderr,
        "%d host_increment_regular() calls: %d + %d = %d. Time spent: "
        "%d ms\n",
        n,
        oldm,
        n,
        m,
        (int)(end - start) / 1000);

    // Execute n ecalls switchlessly
    start = get_relative_time_in_microseconds();
    m = oldm;
    for (int i = 0; i < n; i++)
    {
        oe_result_t result = enclave_decrement_switchless(enclave, &m);
        if (result != OE_OK)
        {
            fprintf(
                stderr, "enclave_decrement_switchless(): result=%u", result);
        }
    }
    end = get_relative_time_in_microseconds();
    fprintf(
        stderr,
        "%d enclave_decrement_switchless() calls: %d - %d = %d. Time spent: "
        "%d ms\n",
        n,
        oldm,
        n,
        m,
        (int)(end - start) / 1000);

    // Execute n regular ecalls
    start = get_relative_time_in_microseconds();
    m = oldm;
    for (int i = 0; i < n; i++)
    {
        oe_result_t result = enclave_decrement_regular(enclave, &m);
        if (result != OE_OK)
        {
            fprintf(stderr, "enclave_decrement_regular(): result=%u", result);
        }
    }
    end = get_relative_time_in_microseconds();
    fprintf(
        stderr,
        "%d enclave_decrement_regular() calls: %d - %d = %d. Time spent: "
        "%d ms\n",
        n,
        oldm,
        n,
        m,
        (int)(end - start) / 1000);

done:
    ret = result != OE_OK ? 1 : 0;
    oe_terminate_enclave(enclave);

    return ret;
}
