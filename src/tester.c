//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <wait.h>
#include <assert.h>
#include "validator_mq.h"
#include "tester_mq.h"

#define SIG_SNT_SUCCESS (SIGRTMIN+1)

static size_t await_responses = 0;
static size_t await_forks = 0;
static pid_t main_pid;
static mqd_t tester_mq;
static struct sigaction snt_success_action;
static comm_sumary_t comm_summary = {0, 0, 0};


/**
 * Error handler.
 * Kills all child processes and exits the current process.
 * Does not affect the validator - this would make one tester failure affect other testers.
 */
void kill_children_exit() {
    exit(EXIT_FAILURE); // TODO
}

/**
 * Error handler.
 * Exits child program with errno return status code.
 */
void exit_with_errno() {
    exit(EXIT_FAILURE); // TODO
}


void snt_success_handler(int sig) {
    assert(sig == SIG_SNT_SUCCESS);
    assert(main_pid == getpid());

    comm_summary.snt++;
}


/**
 * Asynchronous.
 * Sends word as a request to validator, counts fork number for parent process.
 * @param validator_mq
 * @param word
 */
void async_send_request_to_validator(mqd_t validator_mq, const char * word) {
    bool err = false;
    bool start = (word[0]!='!');
    bool halt = !start;

    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Error handling
        case 0:
            close(0); // to prevent data race
            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(kill_children_exit);

            // TODO: Set handler to exit(1) if parent interrupts
            validator_mq_send(validator_mq, start, halt, false, false, main_pid, word, &err);
            HANDLE_ERR(exit_with_errno);
            kill(main_pid, SIG_SNT_SUCCESS);
            exit(EXIT_SUCCESS);
        default:
            await_responses++;
            await_forks++;
    }
}


int main() {
    // initial setup
    bool err = false;
    main_pid = getpid();
    printf("PID: %d\n", main_pid);

    // setup signal handlers TODO: Function
    int tmp_err;
    tmp_err = sigemptyset(&snt_success_action.sa_mask);
    if(tmp_err == -1) {
        exit(EXIT_FAILURE);
    }
    snt_success_action.sa_flags = 0;
    snt_success_action.sa_handler = snt_success_handler;
    tmp_err = sigaction(SIG_SNT_SUCCESS, &snt_success_action, NULL);
    if(tmp_err) {
        exit(EXIT_FAILURE);
    }

    // setup queues
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(main_pid, tester_mq_name);
    tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR(exit_with_errno);

    mqd_t validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
    HANDLE_ERR(exit_with_errno);


    // load words and send them asynchronously
    char buffer[WORD_LEN_MAX+2]; // + '\n' and '\0'
    while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
        assert(buffer[strlen(buffer)-1] == '\n');
        buffer[strlen(buffer)-1] = '\0';
        log_formatted("%d sending: %s", getpid(), buffer);
        async_send_request_to_validator(validator_mq, buffer);
        buffer[0] = '\0'; // following shorter words cannot contain junk
    }
    validator_mq_finish(false, validator_mq, &err);
    HANDLE_ERR(exit_with_errno);

    // process validator responses synchronously
    tester_mq_msg tester_msg;
    while(await_responses) {
        tester_mq_receive(tester_mq, &tester_msg, &err);
        HANDLE_ERR(kill_children_exit);

        await_responses--;
        log_formatted("%d received response for: %s", getpid(), tester_msg.word);
        if(!tester_msg.ignored) {
            comm_summary.rcd++;
            if(tester_msg.accepted) {
                comm_summary.acc++;
                printf("%s A\n", tester_msg.word);
            } else {
                printf("%s N\n", tester_msg.word);
            }
        }
        if(tester_msg.completed) {
            break;
        }
    }
    // clean up
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(kill_children_exit);

    while(await_forks) {
        pid_t tmp_pid = 0;
        int wait_ret = EXIT_SUCCESS;
        tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1 || wait_ret != EXIT_SUCCESS) {
            log_formatted("Unexpected error in one of child processes: %d, %s", errno, strerror(errno));
            kill_children_exit();
        }
        await_forks--;
    }

    print_comm_summary(&comm_summary);
    return 0;
}
