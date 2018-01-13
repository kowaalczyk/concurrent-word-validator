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
    mqd_t tester_mq = tester_mq_start(true);
//    char * response_mq_name = create_response_queue_name(getpid());
//    mqd_t tester_mq = mq_open(response_mq_name, O_RDONLY | O_CREAT);
//    if(tester_mq == -1) {
//        syserr("TESTER: Failed to create response queue");
//    }

    mqd_t validator_mq = validator_mq_start(false);

    char input_buffer[WORD_LEN_MAX];
    char send_buffer[VALIDATOR_MQ_BUFFSIZE];
    while(fgets(input_buffer, sizeof(input_buffer), stdin)) {
        // TODO: Make sure to process empty strings correctly
        async_send_request_to_validator(validator_mq, input_buffer);
    }
    validator_mq_finish(validator_mq);

    while(await_response) {
        // TODO: Wait for responses from validator server
        await_response--;
    }

    // clean queues, requests queue is closed by server (validator)
    if (mq_close(response_mq)) syserr("TESTER: Failed to close queue");
    if (mq_unlink(response_mq_name)) syserr("TESTER: Failed to unlink queue");

    tester_mq_finish(tester_mq);
    while(await_forks) {
        // TODO: Handle errors from forks
        wait(NULL);
        await_forks--;
    }
    return 0;
}
