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

// TODO: Organize variables
static bool err = false;
char tester_mq_name[TESTER_MQ_NAME_LEN];
static size_t await_responses = 0;
static size_t await_forks = 0;
static pid_t main_pid;
static mqd_t tester_mq;
static mqd_t validator_mq;
static struct sigaction snt_success_action;
static comm_sumary_t comm_summary = {0, 0, 0};

// TODO: Signal from validator (custom)

/**
 * Error handler.
 * Kills all child processes and exits the current process.
 * Does not affect the validator - this would make one tester failure affect other testers.
 */
void err_kill_children_exit() {
    kill(0, SIGTERM);
    exit(EXIT_FAILURE); // TODO
}

/**
 * Error handler.
 * Tries to notify parent about the error and
 */
void err_notify_parent() {
    assert(main_pid != getpid());
    fprintf(stderr, "Error, killing children and exiting.\n");
    kill(main_pid, SIGTERM);
}

/**
 * Signal handler for SIG_SNT_SUCCESS.
 * @param sig
 */
void sig_snt_success_handler(int sig) {
    assert(sig == SIG_SNT_SUCCESS);
    assert(main_pid == getpid());

    comm_summary.snt++;
}

/**
 * Signal handler for SIGINT and SIGTERM.
 * @param sig
 */
void sig_err_handler(int sig) {
    if(main_pid == getpid()) {
        // parent process
        // TODO: Attempt to clean everything
        err_kill_children_exit();
    } else {
        // child process does not notify parent - this function is used only to process signals sent from parent
        // TODO: Attempt to clean everything
        exit(EXIT_FAILURE);
    }
}


/**
 * Asynchronous.
 * Sends word as a request to validator, counts fork number for parent process.
 * @param validator_mq
 * @param word
 */
void async_send_request_to_validator(mqd_t validator_mq, const char * word) {
    assert(err == false);

    bool start = (word[0]!='!');
    bool halt = !start;
    switch (fork()) {
        case -1:
            fprintf(stderr, "Unexpected error in fork\n");
            err_kill_children_exit();
        case 0:
            close(0); // to prevent data race
            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(err_kill_children_exit);

            // TODO: Set signal handler for validator interrupt
            validator_mq_send(validator_mq, start, halt, false, false, main_pid, word, &err);
            HANDLE_ERR(err_notify_parent);
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(err_notify_parent);
            kill(main_pid, SIG_SNT_SUCCESS); // TODO: Handle kill error
            exit(EXIT_SUCCESS);
        default:
            await_responses++;
            await_forks++;
    }
}

void setup_sig_handlers(bool *err) {
    int tmp_err;
    tmp_err = sigemptyset(&snt_success_action.sa_mask);
    VOID_FAIL_IF(tmp_err == -1);

    snt_success_action.sa_flags = 0;
    snt_success_action.sa_handler = sig_snt_success_handler;
    tmp_err = sigaction(SIG_SNT_SUCCESS, &snt_success_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);

    // TODO: Setup error handling and validator signal handling
}

void process_response(const tester_mq_msg *msg) {
    if(!msg->ignored) {
        comm_summary.rcd++;
        if(msg->accepted) {
            comm_summary.acc++;
            printf("%s A\n", msg->word);
        } else {
            printf("%s N\n", msg->word);
        }
    }
}

void collect_sender(bool *err) {
    while(await_forks) {
        pid_t tmp_pid = 0;
        int wait_ret = EXIT_SUCCESS;
        tmp_pid = wait(&wait_ret);
        VOID_FAIL_IF(tmp_pid == -1 || wait_ret != EXIT_SUCCESS);
        await_forks--;
    }
}

void read_and_send() {
    char buffer[WORD_LEN_MAX+2]; // + '\n' and '\0'
    while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
        assert(buffer[strlen(buffer)-1] == '\n');

        buffer[strlen(buffer)-1] = '\0';
        bool start = (buffer[0]!='!');
        bool halt = !start;

        log_formatted("%d SND: %s", getpid(), buffer);
        validator_mq_send(validator_mq, start, halt, false, false, main_pid, buffer, &err);
        HANDLE_ERR(err_notify_parent);

        buffer[0] = '\0'; // following shorter words cannot contain junk
    }
}

pid_t async_spawn_sender() {
    pid_t sender_pid;
    switch (sender_pid = fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Handle error
        case 0:
            validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
            HANDLE_ERR_EXIT_WITH_MSG("TESTER: Failed to start validator mq"); // TODO: Sig parent

            read_and_send();

            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(err_kill_children_exit); // TODO: Sig parent
            exit(EXIT_SUCCESS);
        default:
            await_forks++;
            return sender_pid;
    }
}

int main() {
    assert(err == false);
    // initial setup
    main_pid = getpid();
    printf("PID: %d\n", main_pid);
    setup_sig_handlers(&err);
    HANDLE_ERR_EXIT_WITH_MSG("TESTER: Failed to set up signal handlers");

    // setup queues
    tester_mq_get_name_from_pid(main_pid, tester_mq_name);
    tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR_EXIT_WITH_MSG("TESTER: Failed to start tester mq");
//    mqd_t validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
//    HANDLE_ERR_EXIT_WITH_MSG("TESTER: Failed to start validator mq");
//
//    // load words and send them asynchronously
//    char buffer[WORD_LEN_MAX+2]; // + '\n' and '\0'
//    while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
//        assert(buffer[strlen(buffer)-1] == '\n');
//
//        buffer[strlen(buffer)-1] = '\0';
//        log_formatted("%d SND: %s", getpid(), buffer);
//        async_send_request_to_validator(validator_mq, buffer);
//        buffer[0] = '\0'; // following shorter words cannot contain junk
//    }
//    validator_mq_finish(false, validator_mq, &err);
//    HANDLE_ERR(err_kill_children_exit);

    pid_t sender_pid = async_spawn_sender(); // TODO: Global

    // process validator responses
    tester_mq_msg tester_msg;
    while(true) {
        tester_mq_receive(tester_mq, &tester_msg, &err);
        HANDLE_ERR(err_kill_children_exit);

        await_responses--;
        process_response(&tester_msg);
        if(tester_msg.completed) {
            kill(sender_pid, 9); // TODO: change this to something normal
            // completed is received only once, and no messages will be sent from server after that
            break;
        }
    }
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(err_kill_children_exit);

    // clean up
    collect_sender(&err);
    HANDLE_ERR_EXIT_WITH_MSG("TESTER: Unexpected fail while collecting message sender");
    print_comm_summary(&comm_summary);
    return 0;
}
