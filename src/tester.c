//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <mqueue.h>
#include <wait.h>
#include <memory.h>
#include "err.h"
#include "validator_mq.h"
#include "tester_mq.h"

pid_t parent_pid;

size_t await_response = 0;
size_t await_forks = 0;

/**
 * Error handler.
 * Kills all processes and exits the current process.
 * Does not affect the validator - this would make one tester failure affect other testers.
 */
void kill_all_exit() {
    // TODO
}

/**
 * Asynchronous.
 * Sends word as a request to validator
 * @param validator_mq
 * @param word
 */
void async_send_request_to_validator(mqd_t validator_mq, const char * word) {
    bool err;
    switch (fork()) {
        case -1:
            syserr("TESTER: Error in fork");
        case 0:
            validator_mq_send_validation_start_request(validator_mq, word, &err);
            HANDLE_ERR(kill_all_exit);
            exit(0);
        default:
            await_response++;
            await_forks++;
    }
}

int main() {
    bool err = false;
    parent_pid = getpid();

    // setup queues
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(getpid(), tester_mq_name);
    mqd_t tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR(kill_all_exit);

    mqd_t validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
    HANDLE_ERR(kill_all_exit);

    // setup buffer
    char input_buffer[WORD_LEN_MAX];
    size_t request_buffer_len = validator_mq_get_buffsize(validator_mq, &err);
    HANDLE_ERR(kill_all_exit);
    char request_buffer[request_buffer_len];

    // load words and send them asynchronously
    while(fgets(input_buffer, sizeof(input_buffer), stdin)) {
        // TODO: Make sure to process empty strings correctly
        size_t input_word_len = strlen(input_buffer);
        memcpy(request_buffer, input_buffer, input_word_len);
        async_send_request_to_validator(validator_mq, request_buffer);
    }
    validator_mq_finish(validator_mq, &err);
    HANDLE_ERR(kill_all_exit);

    // process validator responses synchronously
    ssize_t response_ret;
    char response_buffer[TESTER_MQ_BUFFSIZE];
    while(await_response) {
        response_ret = tester_mq_receive(tester_mq, response_buffer, TESTER_MQ_BUFFSIZE, &err);
        HANDLE_ERR(kill_all_exit);

        if(tester_mq_received_halt(response_buffer, response_ret)) {
            // TODO: Kill all sending forks

        } else if(tester_mq_received_validation_result(response_buffer, response_ret)) {
            printf("%s", response_buffer); // \n is already present in a response

        } else {
            syserr("TESTER: Received invalid response from validator");
        }
        await_response--;
    }

    // clean up
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(kill_all_exit);

    while(await_forks) {
        // TODO: Handle errors from forks
        wait(NULL);
        await_forks--;
    }
    // TODO: Print report
    return 0;
}
