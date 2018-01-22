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
#include "src/automaton.h"
#include "src/validator_mq.h"
#include "src/tester_mq.h"
#include "src/tester_list.h"
#include "src/pid_list.h"

#ifndef SIG_SNT_SUCCESS
#define SIG_SNT_SUCCESS (SIGRTMIN+1)
#endif /* SIG_SNT_SUCCESS */

static bool err = false;
static bool halt_flag_raised = false;
static size_t await_runs = 0;
static size_t await_forks = 0;

static pid_t main_pid = 0;
static pid_list_item_t *children = NULL;

static automaton a;
static mqd_t validator_mq = -1;
static mqd_t current_tester_mq = -1;
static tester_list_t * tester_data = NULL;

static struct sigaction err_action;
static struct sigaction snt_success_action;

static comm_sumary_t comm_summary = {0, 0, 0};


/**
 * Error handler.
 * Kills all known processes and exits the current process.
 */
static void kill_all_exit() {
    kill(main_pid, SIGTERM);
}

/**
 * Signal handler for SIGINT and SIGTERM.
 * Attempts to clean anything that might be opened (queues, data structures, etc.) within the process,
 * if signal is received in main process of validator, will attempt to forward it to all children.
 * @param sig
 */
static void sig_err_handler(int sig) {
    UNUSED(sig);

    bool ignored_err = false;
    if(main_pid == getpid()) {
        for(tester_list_t *iter = tester_data; iter != NULL; iter = iter->next) {
            if(iter->this != NULL) {
                kill(iter->this->pid, SIGTERM);
            }
        }
        for(pid_list_item_t *iter = children; iter != NULL; iter = iter->next) {
            if(iter->pid != PID_LIST_EMPTY_ITEM_PID) {
                kill(iter->pid, SIGTERM);
            }
        }
        validator_mq_finish(true, validator_mq, &ignored_err);
    } else {
        validator_mq_finish(false, validator_mq, &ignored_err);
    }
    tester_mq_finish(false, current_tester_mq, NULL, &ignored_err);
    // free is not a safe async function => have to rely on system to clean the list's dynamic memory
    exit(EXIT_FAILURE);
}

/**
 * Signal handler for SIG_SNT_SUCCESS.
 * Used to notify main process about successful attempt to send message by a child process.
 * @param sig
 */
static void sig_snt_success_handler(int sig) {
    assert(sig == SIG_SNT_SUCCESS);
    assert(main_pid == getpid());
    UNUSED(sig);

    comm_summary.snt++;
}

/**
 * Sets up signal handlers for the main process.
 * Child process should inherit same signal handlers.
 * Standard error handling.
 * @param err
 */
static void setup_sig_handlers(bool *err) {
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

    snt_success_action.sa_flags = SA_RESTART; // prevent killing mq read when non-killing signal is received
    snt_success_action.sa_handler = sig_snt_success_handler;
    tmp_err = sigaction(SIG_SNT_SUCCESS, &snt_success_action, NULL);
    VOID_FAIL_IF(tmp_err == -1);
}

/**
 * Asynchronous.
 * Launches run process ready to read from provided pipe.
 * Assumes both pipe descriptors are opened.
 * Does not modify pipe in current process.
 * Modifies global err variable in case of error.
 * @param pipe_dsc
 */
static void async_create_run(int *pipe_dsc) {
    assert(!halt_flag_raised);
    assert(err == false);

    ssize_t tmp_err;
    pid_t child_pid;
    switch (child_pid = fork()) {
        case -1:
            err = true;
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            // pipe to child input descriptor
            tmp_err = close(0);
            if(tmp_err) {
                exit(EXIT_FAILURE);
            }
            tmp_err = dup(pipe_dsc[0]);
            if(tmp_err) {
                exit(EXIT_FAILURE);
            }
            tmp_err = close(pipe_dsc[0]);
            if(tmp_err) {
                exit(EXIT_FAILURE);
            }
            tmp_err = close(pipe_dsc[1]);
            if(tmp_err) {
                exit(EXIT_FAILURE);
            }

            // exec child
            char parent_pid_str[PID_STR_LEN];
            sprintf(parent_pid_str, "%d", main_pid);
            char * child_argv[] = {"run", parent_pid_str, NULL};

            execvp("./run", child_argv);
            exit(EXIT_FAILURE);
        default:
            await_forks++;
            await_runs++;
            pid_list_emplace(children, child_pid, &err);
            break;
    }
}

/**
 * Asynchronous.
 * Sends provided automaton and prepared message to run process via specified pipe.
 * Assumes pipe descriptor 0 is closed, and pipe descriptor 1 is opened.
 * Does not modify pipe in current process.
 * Modifies global err variable in case of error.
 * @param pipe_dsc
 * @param a
 * @param prepared_msg
 */
static void async_pipe_data(int *pipe_dsc, const automaton *a, const validator_mq_msg *prepared_msg) {
    assert(!halt_flag_raised);
    assert(err == false);
    // assuming pipe dsc 0 is closed and pipe dsc 1 opened

    ssize_t tmp_err;
    pid_t child_pid;
    switch (child_pid = fork()) {
        case -1:
            err = true;
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            // send automaton
            tmp_err = write(pipe_dsc[1], a, sizeof(automaton));
            if(tmp_err != sizeof(automaton)) {
                kill_all_exit();
            }
            // send prepared message
            tmp_err = write(pipe_dsc[1], prepared_msg, sizeof(validator_mq_msg));
            if(tmp_err != sizeof(validator_mq_msg)) {
                kill_all_exit();
            }
            // close pipe and exit
            tmp_err = close(pipe_dsc[1]);
            if(tmp_err == -1) {
                kill_all_exit();
            }
            exit(EXIT_SUCCESS);
        default:
            await_forks++;
            pid_list_emplace(children, child_pid, &err);
            break;
    }
}

/**
 * Asynchronous.
 * Sends message with provided parameters to tester mq created based on specified tester pid.
 * Modifies global err variable in case of error.
 * @param tester_pid
 * @param word
 * @param completed
 * @param ignored
 * @param accepted
 */
static void async_send_to_tester(pid_t tester_pid, const char *word, bool completed, size_t total_processed, bool ignored,
                          bool accepted) {
    assert(err == false);

    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    pid_t child_pid;
    switch (child_pid = fork()) {
        case -1:
            err = true;
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to finish validator mq in async child");

            tester_mq_get_name_from_pid(tester_pid, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to open tester mq in async child");
            current_tester_mq = tester_mq; // for optimal error handling

            tester_mq_send(tester_mq, word, completed, total_processed, ignored, accepted, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to send to tester mq in async child");
            log_formatted("%d - SND: %s, COMPLETED=%d, IGNORED=%d, ACCEPTED=%d, EXPECTED_RCD_PASSED=%d", getpid(), word, completed, ignored, accepted, total_processed);

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to close tester mq in async child");

            if(word) {
                int tmp_err = kill(main_pid, SIG_SNT_SUCCESS);
                err = (tmp_err == -1);
                HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed notify main process about successful send");
            }
            exit(EXIT_SUCCESS);

        default:
            await_forks++;
            pid_list_emplace(children, child_pid, &err);
            break;
    }
}

/**
 * Performs all pipe and async operations necessary to create a new run process.
 * Uses non-standard error handling in oder to minimize risk of leaving pipes open.
 * Errors are written to global err, and can be caught in a normal way.
 * @param msg
 */
static void init_run(const validator_mq_msg *msg) {
    assert(msg != NULL);

    int tmp_err = 0;
    int pipe_dsc[2];
    tmp_err = pipe(pipe_dsc);
    if(err || tmp_err == -1) {
        err = true;
        return;
    }

    async_create_run(pipe_dsc);
    tmp_err = close(pipe_dsc[0]);
    if(err || tmp_err == -1) {
        err = true;
        close(pipe_dsc[1]);
        return;
    }

    async_pipe_data(pipe_dsc, &a, msg);
    tmp_err = close(pipe_dsc[1]);
    if(err || tmp_err == -1) {
        err = true;
        return;
    }
}

/**
 * Finds or creates tester based on data from request.
 * Handles all errors within the function.
 * @param msg
 * @return
 */
static tester_t *tester_for_request(const validator_mq_msg *msg) {
    tester_t *tester = tester_list_find(tester_data, msg->tester_pid);
    if(!tester) {
        tester = tester_list_emplace(tester_data, msg->tester_pid, 0, 0, 0, false, &err);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to insert tester to list");
    }
    assert(tester != NULL);
    return tester;
}

/**
 * Processes request in context of provided tester and halt lag not raised.
 * Handles all errors within the function.
 * @param tester
 * @param msg
 */
static void handle_request_standard(tester_t *tester, validator_mq_msg *msg) {
    assert(!halt_flag_raised);
    assert(tester != NULL);
    assert(msg != NULL);
    assert(tester->pid == msg->tester_pid);

    log_formatted("%d RCD: %s, completed=%d, finish=%d, for TESTER IN LIST: %d, bal=%d, completed=%d, expected_rcd=%d",
                  getpid(), msg->word, msg->completed, msg->finish,
                  tester->pid, tester->word_bal, tester->completed, tester->rcd);

    if(msg->start) {
        assert(!msg->halt);

        // update logs
        comm_summary.rcd++;
        tester->rcd++;
        tester->word_bal++;
        tester->completed = msg->completed;

        // prepare response
        msg->start = false;
        msg->finish = true;
        msg->accepted = false;

        // perform run asynchronously
        init_run(msg);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to initialize run process");
    } else if(msg->halt) {
        assert(!msg->start);
        assert(msg->completed);

        // update logs
        halt_flag_raised = true;
        tester->completed = true;

        // send completed flag if necessary
        bool send_completed = (tester->word_bal == 0);
        if(send_completed) {
            async_send_to_tester(tester->pid, "THIS IS JUST A FLAG", send_completed, tester->rcd, true, false);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send completed flag to tester");
        }
    } else if(msg->finish) {
        assert(!msg->start);

        // update logs
        await_runs--;
        tester->word_bal--;
        if(msg->accepted) {
            comm_summary.acc++;
            tester->acc++;
        }
        // format and send response
        bool send_completed = (tester->completed && tester->word_bal == 0);
        async_send_to_tester(tester->pid, msg->word, send_completed, tester->rcd, false, msg->accepted);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send message to tester");
    } else {
        assert(!msg->finish && !msg->start && !msg->halt);

        // empty message or completed flag (without word or halt)
        tester->completed = msg->completed;
    }
}

/**
 * Processes request in context of provided tester raised halt flag.
 * All requests other than validation finish are ignored.
 * Handles all errors within the function.
 * @param tester
 * @param msg
 */
static void handle_request_only_finish(tester_t *tester, validator_mq_msg *msg) {
    assert(halt_flag_raised);
    assert(tester != NULL);
    assert(msg != NULL);
    assert(tester->pid == msg->tester_pid);
    assert(tester->word_bal >= 0);

    log_formatted("%d RCD: %s, completed=%d, finish=%d, for TESTER IN LIST: %d, bal=%d, completed=%d, expected_rcd=%d",
                  getpid(), msg->word, msg->completed, msg->finish,
                  tester->pid, tester->word_bal, tester->completed, tester->rcd);

    if(msg->finish) {
        assert(!msg->start);

        // update logs
        await_runs--;
        tester->word_bal--;
        if(msg->accepted) {
            comm_summary.acc++;
            tester->acc++;
        }
        // format and send response
        bool send_completed = (tester->word_bal == 0); // different condition because of raised halt flag
        async_send_to_tester(tester->pid, msg->word, send_completed, tester->rcd, false, msg->accepted);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send response to tester asynchronously");
    } else {
        // no matter what it is, it should be ignored.
        bool send_completed = (tester->word_bal == 0); // different condition because of raised halt flag
        async_send_to_tester(tester->pid, msg->word, send_completed, tester->rcd, true, false);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to ignore tester asynchronously");
    }
}

/**
 * Awaits for forks created for asynchronous tasks.
 * Standard error handling.
 * @param err
 */
static void collect_forks(bool *err) {
    while(await_forks) {
        pid_t tmp_pid = 0;
        int wait_ret = EXIT_SUCCESS;
        tmp_pid = wait(&wait_ret);
        VOID_FAIL_IF(tmp_pid == -1 || wait_ret != EXIT_SUCCESS);
        await_forks--;
    }
}

int main() {
    assert(err == false);
    // initial setup
    main_pid = getpid();
    children = pid_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to create list of children pids");
    setup_sig_handlers(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to set up signal handlers");
    tester_data = tester_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to create tester list");
    load_automaton(&a, &err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to load automaton");

    // setup queues
    validator_mq = validator_mq_start(true, &err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to start validator MQ");

    // handle incoming requests
    validator_mq_msg validator_msg;
    while(!halt_flag_raised) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to receive message from validator mq");

        tester_t *tester = tester_for_request(&validator_msg);
        handle_request_standard(tester, &validator_msg);
    }
    // after halt flag is raised, wait for runs to complete and ignore other testers if necessary
    while(await_runs) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to receive message from validator mq");

        tester_t *tester = tester_for_request(&validator_msg);
        handle_request_only_finish(tester, &validator_msg);
    }
    // clean up
    collect_forks(&err);
    HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to collect forks created for async tasks");

    validator_mq_finish(true, validator_mq, &err);
    HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to close validator MQ");

    print_comm_summary(&comm_summary);
    tester_list_print_log(tester_data);
    tester_list_destroy(tester_data);
    pid_list_destroy(children);
    return 0;
}
