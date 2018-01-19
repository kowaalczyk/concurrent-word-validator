//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <wait.h>
#include "validator_mq.h"
#include "tester_mq.h"

pid_t main_pid;
size_t await_forks = 0;

// TODO: Signal handlers

/**
 * Error handler.
 * Kills all processes and exits the current process.
 * Does not affect the validator - this would make one tester failure affect other testers.
 */
void kill_all_exit() {
    // TODO
}

/**
 * Error handler.
 * Exits child program with errno return status code.
 */
void exit_with_errno() {
    // TODO: Clean and log
    exit(errno);
}

// TODO: Forks should immediately close tester_mq

/**
 * Asynchronous.
 * Sends word as a request to validator, counts fork number for parent process.
 * Child process will return 0 if sent successfully, negative value if an error occured,
 * and positive value if it was interrupted by parent.
 * @param validator_mq
 * @param word
 */
void async_send_request_to_validator(mqd_t validator_mq, const char * word) {
    bool err = false;
    switch (fork()) {
        case -1:
//            syserr("TESTER: Error in fork");
            break;
        case 0:
            // TODO: Set handler to exit(1) if parent interrupts
            validator_mq_send_validation_start_request(validator_mq, word, &err);
            HANDLE_ERR(exit_with_errno);
            exit(0);
        default:
            await_forks++;
    }
}

struct comm_summary{
    size_t snt;
    size_t rcd;
    size_t acc;
};

int main() {
    bool err = false;
    main_pid = getpid();
    struct comm_summary comm_summary = {0, 0, 0};

    // setup queues
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(main_pid, tester_mq_name);
    mqd_t tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR(exit_with_errno);

    mqd_t validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
    HANDLE_ERR(exit_with_errno);

    // setup buffer
    char input_buffer[WORD_LEN_MAX];
    size_t request_buffer_len = validator_mq_get_buffsize(validator_mq, &err);
    HANDLE_ERR(exit_with_errno);
    char request_buffer[request_buffer_len];

    // load words and send them asynchronously
    while(fgets(input_buffer, sizeof(input_buffer), stdin)) {
        size_t input_word_len = strlen(input_buffer);
        memcpy(request_buffer, input_buffer, input_word_len);
        async_send_request_to_validator(validator_mq, request_buffer);
    }
    validator_mq_finish(validator_mq, false, &err);
    HANDLE_ERR(exit_with_errno);

    // process validator responses synchronously
    tester_mq_msg response_msg;
    while(true) {
        tester_mq_receive(tester_mq, &response_msg, &err);
        HANDLE_ERR(kill_all_exit);

        comm_summary.rcd++;
        if(!response_msg.ignored) {
            if(response_msg.accepted) {
                comm_summary.acc++;
                printf("%s A\n", response_msg.word);
            } else {
                printf("%s N\n", response_msg.word);
            }
        }
        if(response_msg.completed) {
            break;
        }
    }
    // clean up
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(kill_all_exit);

    while(await_forks) {
        pid_t tmp_pid;
        int wait_ret;
        tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1) {
            kill_all_exit();
        }
        if(wait_ret == 0) {
            comm_summary.snt++;
        }
        await_forks--;
    }
    // print comm summary
    printf("Snt: %zu\nRcd: %zu\nAcc: %zu\n", comm_summary.snt, comm_summary.rcd, comm_summary.acc);
    return 0;
}
