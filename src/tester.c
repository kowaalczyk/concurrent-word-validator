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


size_t await_response = 0;
size_t await_forks = 0;

/**
 * Asynchronous.
 * Sends word as a request to validator
 * @param validator_mq
 * @param word
 */
void async_send_request_to_validator(mqd_t validator_mq, const char * word) {
    switch (fork()) {
        case -1:
            syserr("TESTER: Error in fork");
        case 0:
            validator_mq_send_validation_start_request(validator_mq, word);
            exit(0);
        default:
            await_response++;
            await_forks++;
    }
}

int main() {
    // setup queues
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(getpid(), tester_mq_name);
    mqd_t tester_mq = tester_mq_start(true, tester_mq_name);
    mqd_t validator_mq = validator_mq_start(false); // assuming there is only one validator,

    // setup buffer
    char input_buffer[WORD_LEN_MAX];
    char request_buffer[VALIDATOR_MQ_BUFFSIZE];

    // load words and send them asynchronously
    while(fgets(input_buffer, sizeof(input_buffer), stdin)) {
        // TODO: Make sure to process empty strings correctly
        memcpy(request_buffer, input_buffer, strlen(input_buffer));
        async_send_request_to_validator(validator_mq, request_buffer);
    }
    validator_mq_finish(validator_mq);

    // process validator responses synchronously
    ssize_t response_ret;
    char response_buffer[TESTER_MQ_BUFFSIZE];
    while(await_response) {
        response_ret = tester_mq_receive(tester_mq, response_buffer, TESTER_MQ_BUFFSIZE);
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
    tester_mq_finish(tester_mq);
    while(await_forks) {
        // TODO: Handle errors from forks
        wait(NULL);
        await_forks--;
    }
    return 0;
}
