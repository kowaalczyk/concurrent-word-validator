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
#define SIG_RCD_COMPLETE (SIGRTMIN+2)

static bool err = false;
static bool await_sender = false;

static pid_t main_pid = 0;
static pid_t sender_pid = 0;

static char tester_mq_name[TESTER_MQ_NAME_LEN];
static mqd_t tester_mq = -1;
static mqd_t validator_mq = -1;

static struct sigaction err_action;
static struct sigaction snt_success_action;
static struct sigaction rcd_complete_action;

static comm_sumary_t comm_summary = {0, 0, 0};

// TODO: Signal from validator (custom)

/**
 * Error handler.
 * When executed, signals
 */
void err_sig_other_and_exit() {
    pid_t other = getpid() == main_pid ? sender_pid : main_pid;
    kill(other, SIGTERM);
    exit(EXIT_FAILURE);
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
 * Signal handler for SIG_RCD_COMPLETE
 * @param sig
 */
void sig_rcd_complete_handler(int sig) {
    assert(sender_pid == getpid());

    validator_mq_finish(false, validator_mq, &err); // fail silently
    exit(EXIT_SUCCESS);
}

/**
 * Signal handler for SIGINT and SIGTERM.
 * Attempts to clean anything that might be opened (queues, data structures, etc.) within the process,
 * and if main process is the one receiving the signal, it passes it onto the sender.
 * All without error checking, as it is no longer necessary and possible.
 * @param sig
 */
void sig_err_handler(int sig) {
    if(main_pid == getpid()) {
        // main process
        validator_mq_finish(false, validator_mq, &err);
        HANDLE_ERR_PRINT_MSG("Failed to finish validator mq");
        tester_mq_finish(true, tester_mq, tester_mq_name, &err);
        HANDLE_ERR_PRINT_MSG(tester_mq_name);
        if(sender_pid != 0) {
            kill(sender_pid, SIGTERM);
        }
        exit(EXIT_FAILURE);
    } else {
        // sender process
        validator_mq_finish(false, validator_mq, &err);
        HANDLE_ERR_PRINT_MSG("Failed to finish validator mq");
        tester_mq_finish(false, tester_mq, NULL, &err);
        HANDLE_ERR_PRINT_MSG("Failed to finish tester mq");
        exit(EXIT_FAILURE);
    }
}

/**
 * Sets up signal handlers for the main process.
 * Sender process should inherit same signal handlers.
 * Standard error handling.
 * @param err
 */
void setup_sig_handlers(bool *err) {
    assert(main_pid == getpid());

    int tmp_err;

    // error handler
    tmp_err = sigemptyset(&err_action.sa_mask);
    VOID_FAIL_IF(tmp_err == -1);

    err_action.sa_flags = 0;
    err_action.sa_handler = sig_err_handler;
    tmp_err = sigaction(SIGINT, &err_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);

    tmp_err = sigaction(SIGTERM, &err_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);

    // snt success handler
    tmp_err = sigemptyset(&snt_success_action.sa_mask);
    VOID_FAIL_IF(tmp_err == -1);

    snt_success_action.sa_flags = 0;
    snt_success_action.sa_handler = sig_snt_success_handler;
    tmp_err = sigaction(SIG_SNT_SUCCESS, &snt_success_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);

    // rcd complete handler
    tmp_err = sigemptyset(&rcd_complete_action.sa_mask);
    VOID_FAIL_IF(tmp_err == -1);

    rcd_complete_action.sa_flags = 0;
    rcd_complete_action.sa_handler = sig_rcd_complete_handler;
    tmp_err = sigaction(SIG_RCD_COMPLETE, &snt_success_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);
}

/**
 * Processes single message from tester mq.
 * @param msg
 */
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

/**
 * Waits for sender to finish working and checks its exit code.
 * Standard error handling.
 * @param err
 */
void collect_sender(bool *err) {
    assert(await_sender == true);

    pid_t tmp_pid = 0;
    int wait_ret = EXIT_SUCCESS;
    tmp_pid = wait(&wait_ret);
    VOID_FAIL_IF(tmp_pid == -1 || wait_ret != EXIT_SUCCESS);
    await_sender = false;
}

/**
 * Blocking.
 * Reads all data in stdin line by line, after reading each line performs blocking send to validator mq.
 * Standard error handling.
 * @param err
 */
void read_and_send(bool *err) {
    int tmp_err = 0;
    bool start = true;
    bool halt = !start;
    char buffer[WORD_LEN_MAX+2]; // + '\n' and '\0'
    while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
        assert(buffer[strlen(buffer)-1] == '\n');

        buffer[strlen(buffer)-1] = '\0';
        start = (buffer[0]!='!');
        halt = !start;

        log_formatted("%d SND: %s", getpid(), buffer);
        validator_mq_send(validator_mq, start, halt, halt, false, false, main_pid, buffer, err); // halt => completed
        VOID_FAIL_IF(*err); // pass error further

        tmp_err = kill(main_pid, SIG_SNT_SUCCESS);
        VOID_FAIL_IF(tmp_err == -1);

        buffer[0] = '\0'; // following shorter words cannot contain junk
        if(halt) {
            break; // no point in sending extra words
        }
    }
    if(!halt) {
        // halt => completed, so there is no point in sending second completed message
        validator_mq_send(validator_mq, false, false, true, false, false, main_pid, buffer, err);
        VOID_FAIL_IF(*err); // pass error further
    }
}

/**
 * Asynchronous.
 * Creates sender process that will read words from stdin and send them to validator.
 * If any of the processes (main or sender) fails, other one is notified and both are expected to exit asap.
 */
void async_spawn_sender() {
    assert(err == false);
    assert(main_pid != 0);

    switch (sender_pid = fork()) {
        case -1:
            HANDLE_ERR_EXIT_WITH_MSG("Error in fork");
        case 0:
            // close tester mq
            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to finish tester mq in sender process");

            // send messages via validator mq
            validator_mq = validator_mq_start(false, &err); // assuming there is only one validator
            HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to start validator mq");

            read_and_send(&err);
            HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to read/send word to validator mq");

            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to finish validator mq");
            exit(EXIT_SUCCESS);
        default:
            await_sender = true;
            break;
    }
}

int main() {
    assert(err == false);
    // initial setup
    main_pid = getpid();
    printf("PID: %d\n", main_pid);
    setup_sig_handlers(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to set up signal handlers");

    // setup queues
    tester_mq_get_name_from_pid(main_pid, tester_mq_name);
    tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to start tester mq");

    // create process for processing and sending words
    async_spawn_sender();
    close(0); // prevent any possible data races in input

    // process validator responses
    tester_mq_msg tester_msg;
    while(true) {
        tester_mq_receive(tester_mq, &tester_msg, &err);
        HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to receive message from tester mq");

        process_response(&tester_msg);
        if(tester_msg.completed) {
            kill(sender_pid, SIG_RCD_COMPLETE);
            // completed is received exactly once (unless an error occured), and no following messages are sent
            break;
        }
    }
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR_WITH_MSG(err_sig_other_and_exit, "Failed to finish tester mq in main process");

    // clean up
    collect_sender(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Unexpected fail while collecting message sender");
    print_comm_summary(&comm_summary);
    return 0;
}
