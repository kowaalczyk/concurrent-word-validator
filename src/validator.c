//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <stdbool.h>
#include <zconf.h>
#include <wait.h>
#include <assert.h>
#include "automaton.h"
#include "err.h"
#include "validator_mq.h"
#include "tester_mq.h"


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

// TODO: Make sure multiple opening and closing tester_mqs in forks is working, if not implement vector of mqd_t-s w/ names

/**
 * Asynchronous.
 * Forwards validation finished request to proper tester as a response.
 * @param buffer - buffer to which validation finish request was received
 * @param buffer_size - actual length of data in buffer
 */
void async_forward_response(const char * buffer, ssize_t buffer_size) {
    char pid_msg_part[PID_STR_LEN];
    char word_msg_part[WORD_LEN_MAX];
    char flag_msg_part;
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;

    switch (fork()) {
        case -1:
            syserr("VALIDATOR: Error in fork");
        case 0:
            validator_mq_extract_pidstr(buffer, pid_msg_part);
            validator_mq_extract_word(buffer, word_msg_part);
            validator_mq_extract_flag(buffer, &flag_msg_part);

            tester_mq_get_name_from_pidstr(pid_msg_part, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name);
            tester_mq_send_validation_result(tester_mq, word_msg_part, &flag_msg_part);
            tester_mq_finish(tester_mq);

            // TODO: update local logs
            exit(0);
        default:
            await_forks++;
            await_runs--;
    }
}

void async_signal_halt(const char * buffer, ssize_t buffer_size) {
    char pid_msg_part[PID_STR_LEN];
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;

    switch (fork()) {
        case -1:
            syserr("VALIDATOR: Error in fork");
        case 0:
            validator_mq_extract_pidstr(buffer, pid_msg_part);
            tester_mq_get_name_from_pidstr(pid_msg_part, tester_mq_name);

            tester_mq = tester_mq_start(false, tester_mq_name);
            tester_mq_send_halt(tester_mq);
            tester_mq_finish(tester_mq);

            // TODO: update local logs
            exit(0);
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
    while(!halt_flag_raised) {
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
    // after halt flag is raised, wait for runs to complete and halt other testers if necessary
    while(await_runs) {
        request_ret = validator_mq_receive(request_mq, request_buff, VALIDATOR_MQ_BUFFSIZE);

        // process request
        if(validator_mq_requested_validation_finish(request_buff, request_ret)) {
            async_forward_response(request_buff, request_ret);

        } else {
            assert(validator_mq_requested_halt(request_buff, request_ret) || validator_mq_requested_validation_start(request_buff, request_ret));
            async_signal_halt(request_buff, request_ret);
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
