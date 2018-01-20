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
size_t await_responses = 0;

// TODO: Signal handlers

/**
 * Error handler.
 * Kills all child processes and exits the current process.
 * Does not affect the validator - this would make one tester failure affect other testers.
 */
void kill_children_exit() {
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
    bool start = (word[0]=='!');
    bool halt = !start;
    size_t word_len;

    switch (fork()) {
        case -1:
            // TODO: Error handling
            break;
        case 0:
            // TODO: Set handler to exit(1) if parent interrupts
            // TODO: Close opened MQs
            validator_mq_send(validator_mq, start, halt, false, false, main_pid, word, &err);
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
    // initial setup
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


    // load words and send them asynchronously
    char buffer[WORD_LEN_MAX+2]; // + '\n' and EOF
    while(fgets(buffer, sizeof(buffer), stdin)) {
        async_send_request_to_validator(validator_mq, buffer);
    }
    validator_mq_finish(false, validator_mq, &err);
    HANDLE_ERR(exit_with_errno);

    // process validator responses synchronously
    tester_mq_msg response_msg;
    while(await_responses) {
        tester_mq_receive(tester_mq, &response_msg, &err);
        HANDLE_ERR(kill_children_exit);

        comm_summary.rcd++; // TODO: Function
        if(!response_msg.ignored) {
            await_responses--;
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
    HANDLE_ERR(kill_children_exit);

    while(await_forks) {
        pid_t tmp_pid;
        int wait_ret;
        tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1) {
            kill_children_exit();
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
