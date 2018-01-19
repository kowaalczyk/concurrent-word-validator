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
#include <errno.h>
#include "automaton.h"
#include "err.h"
#include "validator_mq.h"
#include "tester_mq.h"
#include "tester_list.h"


bool halt_flag_raised = false;
size_t await_runs = 0;
size_t await_forks = 0;

/**
 * Error handler.
 * Raises halt flag, without exiting.
 */
void raise_halt_flag() {
    halt_flag_raised = true;
}

/**
 * Error handler.
 * Exits child program with errno return status code.
 */
void exit_with_errno() {
    // TODO: Clean and log
    exit(errno);
}

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

// TODO: Forks should immediately close validator mq
// TODO: Make sure multiple opening and closing tester_mqs in forks is working, if not implement vector of mqd_t-s w/ names

/**
 * Asynchronous.
 * Forwards validation finished request to proper tester as a response.
 * @param buffer - buffer to which validation finish request was received
 * @param buffer_size - actual length of data in buffer
 */
void async_forward_response(const char * buffer, ssize_t buffer_size) {
    bool err = false;
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
            tester_mq = tester_mq_start(false, tester_mq_name, &err);
            HANDLE_ERR(exit_with_errno);
            tester_mq_send_validation_result(tester_mq, word_msg_part, &flag_msg_part, &err);
            HANDLE_ERR(exit_with_errno);
            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(exit_with_errno);

            // TODO: update local logs
            exit(0);
        default:
            await_forks++;
            await_runs--;
    }
}

/**
 * Asynchronous.
 * Send halt signal as a response to message (provided as buffer).
 * @param buffer - buffer that contains message with extractable pid of sender
 * @param buffer_size - actual length of data in buffer
 */
void async_signal_halt(const char * buffer, ssize_t buffer_size) {
    bool err = false;
    char pid_msg_part[PID_STR_LEN];
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;

    switch (fork()) {
        case -1:
            syserr("VALIDATOR: Error in fork");
        case 0:
            validator_mq_extract_pidstr(buffer, pid_msg_part);
            tester_mq_get_name_from_pidstr(pid_msg_part, tester_mq_name);

            tester_mq = tester_mq_start(false, tester_mq_name, &err);
            HANDLE_ERR(exit_with_errno);
            tester_mq_send_halt(tester_mq, &err);
            HANDLE_ERR(exit_with_errno);
            tester_mq_finish(false, tester_mq, tester_mq_name, &err);
            HANDLE_ERR(exit_with_errno);

            // TODO: update local logs
            exit(0);
        default:
            await_forks++;
            await_runs--;
    }
}

int main() {
    bool err = false;
    // setup
    const automaton * a = load_automaton();
    char request_buff[VALIDATOR_MQ_BUFFSIZE];
    ssize_t request_ret;
    mqd_t request_mq = validator_mq_start(true, &err);
    HANDLE_ERR_EXIT_WITH_MSG("Could not start validator mq");
    tester_list_t * tester_data = tester_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Could create tester list");

    // handle incoming requests
    while(!halt_flag_raised) {
        request_ret = validator_mq_receive(request_mq, request_buff, VALIDATOR_MQ_BUFFSIZE, &err);
        HANDLE_ERR(raise_halt_flag);

        // process request
        if(validator_mq_requested_halt(request_buff, request_ret)) {
            halt_flag_raised = true;

        } else if(validator_mq_requested_validation_finish(request_buff, request_ret)) {
            // Update local logs and forward response TODO: Function
            pid_t tester_pid = validator_mq_extract_pid(request_buff);
            tester_t * tester = tester_list_find(tester_data, tester_pid);
            assert(tester != NULL); // TODO: Really check this
            if(validator_mq_validation_passed(request_buff)) {
                tester->acc += 1;
            }
            async_forward_response(request_buff, request_ret); // TODO: Use found pid for faster sending

        } else if(validator_mq_requested_validation_start(request_buff, request_ret)) {
            // Update local logs and start validation TODO: Function
            pid_t tester_pid = validator_mq_extract_pid(request_buff);
            tester_t * tester = tester_list_find(tester_data, tester_pid);
            if(tester) {
                tester->rcd += 1;
            } else {
                tester_list_emplace(tester_data, tester_pid, 1, 0, &err);
                HANDLE_ERR(raise_halt_flag);
            }
            async_start_validation(request_buff, request_ret);

        } else {
            syserr("VALIDATOR: Received invalid request");
        }
    }
    // after halt flag is raised, wait for runs to complete and halt other testers if necessary
    while(await_runs) {
        request_ret = validator_mq_receive(request_mq, request_buff, VALIDATOR_MQ_BUFFSIZE, &err);
        HANDLE_ERR_DECREMENT_CONTINUE(await_runs);

        // process request
        if(validator_mq_requested_validation_finish(request_buff, request_ret)) {
            async_forward_response(request_buff, request_ret);

        } else {
            assert(validator_mq_requested_halt(request_buff, request_ret) || validator_mq_requested_validation_start(request_buff, request_ret));
            async_signal_halt(request_buff, request_ret);
        }
    }

    // clean up
    validator_mq_finish(request_mq, true, &err);
    HANDLE_ERR(exit_with_errno); // TODO: Really check this
    free((void *) a);
    while(await_forks) {
        // TODO: Handle errors from forks
        wait(NULL);
        await_forks--;
    }
    // TODO: Complete validator logs
    tester_list_print_log(tester_data);
    tester_list_destroy(tester_data);
    return 0;
}
