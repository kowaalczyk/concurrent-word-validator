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
#include "validator_mq.h"
#include "tester_mq.h"
#include "tester_list.h"

#define SIG_SNT_SUCCESS (SIGRTMIN+1)
// TODO: Signal to tester (custom)

// TODO: Organize variables
static bool err = false;
static bool halt_flag_raised = false;
static comm_sumary_t comm_summary = {0, 0, 0};
static size_t await_runs = 0;
static size_t await_forks = 0;
static mqd_t validator_mq;
static pid_t main_pid;

/**
 * Error handler.
 * Raises halt flag, without exiting.
 */
void notify_parent() {
    assert(err == true);
    kill(main_pid, SIGTERM);
    halt_flag_raised = true;
}

/**
 * Error handler.
 * Kills all known processes and exits the current process.
 */
void kill_all_exit() {
    assert(err == true);
    exit(EXIT_FAILURE); // TODO
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
 * Asynchronous.
 * Launches run process ready to read from provided pipe.
 * Assumes both pipe descriptors are opened.
 * Does not modify pipe in current process.
 * @param pipe_dsc
 */
void async_create_run(int *pipe_dsc) {
    assert(!halt_flag_raised);
    assert(err == false);

    ssize_t tmp_err;
    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Error handling
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            // pipe to child input descriptor
            tmp_err = close(0);
            if(tmp_err) {
                exit(EXIT_FAILURE); // TODO: Error handling
            }
            tmp_err = dup(pipe_dsc[0]);
            if(tmp_err) {
                exit(EXIT_FAILURE); // TODO: Error handling
            }
            tmp_err = close(pipe_dsc[0]);
            if(tmp_err) {
                exit(EXIT_FAILURE); // TODO: Error handling
            }
            tmp_err = close(pipe_dsc[1]);
            if(tmp_err) {
                exit(EXIT_FAILURE); // TODO: Error handling
            }
            // exec child
            char * child_argv[] = {"run", NULL};
            execvp("./run", child_argv);
            exit(EXIT_FAILURE); // TODO: Error handling
        default:
            await_forks++;
            await_runs++;
            break;
    }
}

/**
 * Asynchronous.
 * Sends provided automaton and prepared message to run process via specified pipe.
 * Assumes pipe descriptor 0 is closed, and pipe descriptor 1 is opened.
 * Does not modify pipe in current process.
 * @param pipe_dsc
 * @param a
 * @param prepared_msg
 */
void async_pipe_data(int *pipe_dsc, const automaton *a, const validator_mq_msg *prepared_msg) {
    assert(!halt_flag_raised);
    assert(err == false);
    // assuming pipe dsc 0 is closed and pipe dsc 1 opened

    ssize_t tmp_err;
    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Error handling
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            // send automaton
            tmp_err = write(pipe_dsc[1], a, sizeof(automaton));
            if(tmp_err != sizeof(automaton)) {
                kill_all_exit(); // TODO: Error handling
            }
            // send prepared message
            tmp_err = write(pipe_dsc[1], prepared_msg, sizeof(validator_mq_msg));
            if(tmp_err != sizeof(validator_mq_msg)) {
                kill_all_exit(); // TODO: Error handling
            }
            // close pipe and exit
            tmp_err = close(pipe_dsc[1]);
            if(tmp_err == -1) {
                kill_all_exit(); // TODO: Error handling
            }
            exit(EXIT_SUCCESS);
        default:
            await_forks++;
            break;
    }
}

/**
 * Asynchronous.
 * Processes request from run and sends it to specified tester's mq.
 * @param tester
 * @param request_msg
 */
void async_forward_response(const tester_t *tester, const validator_mq_msg *request_msg) {
    assert(err == false);

    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    bool completed = (halt_flag_raised && tester->word_bal == 0);
    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Error handling
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            if(completed) {
                HANDLE_ERR(kill_all_exit);
            } else {
                exit(EXIT_SUCCESS); // not critical - fail silently
            }

            tester_mq_get_name_from_pid(tester->pid, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err); //TODO: ENOENT (tester closed mq) thrown here!
            HANDLE_ERR(kill_all_exit);

            tester_mq_send(tester_mq, request_msg->word, completed, false, request_msg->accepted, &err);
            HANDLE_ERR(kill_all_exit);
            if(completed) {
                log_formatted("COMPLETED!!!");
            }

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(kill_all_exit);
            // TODO: kill return
            exit(EXIT_SUCCESS);

        default:
            await_forks++;
            await_runs--;
            break;
    }
}

/**
 * Asynchronous.
 * Sends ignored flag, meaning that the word will never be processed, to the tester.
 * @param tester
 * @param request_msg
 */
void async_reply_ignore(const tester_t *tester, const validator_mq_msg *request_msg) {
    assert(err == false);

    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    bool completed = (halt_flag_raised && tester->word_bal == 0);
    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE); // TODO: Error handling
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_get_name_from_pid(tester->pid, tester_mq_name);
            log_formatted("Opening tester mq in reply ignore: %s", tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err); //TODO: ENOENT (tester closed mq) thrown here!
            if(completed) {
                HANDLE_ERR(kill_all_exit);
            } else {
                exit(EXIT_SUCCESS); // not critical - fail silently
            }

            tester_mq_send(tester_mq, request_msg->word, completed, true, false, &err);
            HANDLE_ERR(kill_all_exit);
            log_formatted("COMPLETED!!!");

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(kill_all_exit);
            exit(EXIT_SUCCESS);

        default:
            await_forks++;
            break;
    }
}


int main() {
    assert(err == false);
    // initial setup
    main_pid = getpid();
    tester_list_t * tester_data = tester_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("VALIDATOR: Could not create tester list");
    automaton a;
    load_automaton(&a, &err);
    HANDLE_ERR_EXIT_WITH_MSG("VALIDATOR: Failed to load automaton");

    // setup queues
    validator_mq = validator_mq_start(true, &err);
    HANDLE_ERR_EXIT_WITH_MSG("VALIDATOR: Could not start validator mq");

    // handle incoming requests
    validator_mq_msg validator_msg;
    while(!halt_flag_raised) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR(notify_parent);

        // process request
        if(validator_msg.halt) {
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(!tester) {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 0, 0, 0, &err);
                HANDLE_ERR(notify_parent);
            }
            halt_flag_raised = true;
            // TODO: Notify known testers

        } else if(validator_msg.finished) {
            // Update local logs and forward response TODO: Function
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            assert(tester != NULL);
            tester->word_bal--;
            if(validator_msg.accepted) {
                comm_summary.acc++;
                tester->acc++;
            }
            async_forward_response(tester, &validator_msg);

        } else if(validator_msg.start) {
            log_formatted("RCD: %s", validator_msg.word);
            // TODO: Function
            // update local logs
            comm_summary.rcd++;
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(tester) {
                tester->word_bal++;
                tester->rcd++;
            } else {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 1, 0, 1, &err);
                HANDLE_ERR(notify_parent);
            }
            // prepare validator message
            validator_msg.start = false;
            validator_msg.halt = false;
            validator_msg.finished = true;
            validator_msg.accepted = false;

            int pipe_dsc[2];
            pipe(pipe_dsc); // TODO: Handle err

            async_create_run(pipe_dsc);
            close(pipe_dsc[0]); // TODO: Handle err

            async_pipe_data(pipe_dsc, &a, &validator_msg);
            close(pipe_dsc[1]); // TODO: Handle err

        } else {
            // TODO: Consider handling this
        }
    }
    // after halt flag is raised, wait for runs to complete and ignore other testers if necessary
    while(await_runs) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR_DECREMENT_CONTINUE(await_runs);

        // process request
        if(validator_msg.finished) {
            // Update local logs and forward response TODO: Function
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            assert(tester != NULL);
            tester->word_bal--;
            if(validator_msg.accepted) {
                comm_summary.acc++;
                tester->acc++;
            }
            async_forward_response(tester, &validator_msg);

        } else {
            log_formatted("RCD_IGN: %s", validator_msg.word);
            // only happens when one tester send halt and other tester completed request before receiving signal
            if(validator_msg.start) {
                comm_summary.rcd++;
            }
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(tester) {
                tester->rcd += 1;
                if(tester->word_bal == 0) {
                    // completed message has already been passed to tester
                } else {
                    async_reply_ignore(tester, &validator_msg);
                }
            } else {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 1, 0, 0, &err);
                HANDLE_ERR(notify_parent);
                async_reply_ignore(tester, &validator_msg);
            }
        }
    }

    // clean up
    validator_mq_finish(true, validator_mq, &err);
    HANDLE_ERR(kill_all_exit);
    while(await_forks) {
        pid_t tmp_pid = 0;
        int wait_ret = EXIT_SUCCESS;
        tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1 || wait_ret != EXIT_SUCCESS) {
            log_formatted("Unexpected error in one of child processes: %d, %s", errno, strerror(errno));
            kill_all_exit();
        }
        await_forks--;
    }
    print_comm_summary(&comm_summary);
    tester_list_print_log(tester_data);
    tester_list_destroy(tester_data);
    return 0;
}
