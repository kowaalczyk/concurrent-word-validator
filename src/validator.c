//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <stdbool.h>
#include <zconf.h>
#include <wait.h>
#include "automaton.h"
#include "err.h"
#include "validator_mq.h"


bool halt_flag_raised = false;
size_t await_runs = 0;
size_t await_forks = 0;

/**
 * Asynchronous.
 * Creates run process that will validate word from request.
 * @param buffer - buffer to which validation start request was received
 * @param buffer_size - actual length of data in buffer
 */
void async_start_validation(const char *buffer, ssize_t buffer_size) {
    char word[WORD_LEN_MAX];

    switch (fork()) {
        case -1:
            syserr("VALIDATOR: Error in fork");
        case 0:
            // child creates run, sends automaton via mq, closes it and exits
//            get_request_word(buffer, buffer_size, word);
            // TODO: Create MQ for sending automaton and word to run
            // TODO: Create run
            // TODO: Send automaton to run
            // TODO: Send word to run
            // TODO: Close MQ to run
            exit(0);
        default:
            // TODO: Create response MQ and open it for writing (this is non-blocking, tester already has an open queue)
            await_forks++;
            await_runs++;
    }
}

/**
 * Asynchronous.
 * Forwards validation finished request to proper tester as a response.
 * @param buffer - buffer to which validation finish request was received
 * @param buffer_size - actual length of data in buffer
 */
void async_forward_response(const char * buffer, ssize_t buffer_size) {
    switch (fork()) {
        case -1:
            syserr("VALIDATOR: Error in fork");
        case 0:
            // TODO: Parse buffer if necessary
            // TODO: child process get response MQ mqd_t,
            // TODO: send response,
            // TODO: update local logs
            break;
        default:
            await_forks++;
            await_runs--;
    }
}

int main() {
    // setup
    const automaton * a = load_automaton();
    char request_buff[VALIDATOR_MQ_BUFFSIZE];
    ssize_t request_ret;
    mqd_t request_mq = validator_mq_start(true); // 1=> server, 0=> client
    // handle incoming requests
    while(!halt_flag_raised || await_runs) {
        // wait for incoming request
        request_ret = validator_mq_receive(request_mq, request_buff, VALIDATOR_MQ_BUFFSIZE);
        // process request
        if(validator_mq_requested_halt(request_buff, request_ret)) {
            halt_flag_raised = true;

        } else if(validator_mq_requested_validation_finish(request_buff, request_ret)) {
            async_forward_response(request_buff, request_ret);

        } else if(validator_mq_requested_validation_start(request_buff, request_ret)) {
            async_start_validation(request_buff, request_ret);

        } else {
            syserr("VALIDATOR: Received invalid request");
        }
    }
    // clean up
    validator_mq_finish(request_mq);
    free((void *) a);
    while(await_forks) {
        // TODO: Handle errors from forks
        wait(NULL);
        await_forks--;
    }
    return 0;
}
