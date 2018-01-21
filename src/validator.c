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
 * Kills all known processes and exits the current process.
 */
void kill_all_exit() {
    assert(err == true);
    // TODO: Kill validator pids
    kill(0, SIGTERM);
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

// TODO: Handle sigint and sigterm by clearing own resources and exiting with error


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
 * Sends message with provided parameters to tester mq created based on specified tester pid.
 * Modifies global err variable in case of error.
 * @param tester_pid
 * @param word
 * @param completed
 * @param ignored
 * @param accepted
 */
void async_send_to_tester(pid_t tester_pid, const char *word, bool completed, bool ignored, bool accepted) {
    assert(err == false);

    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    switch (fork()) {
        case -1:
            err = true;
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to finish validator mq in async child");

            tester_mq_get_name_from_pid(tester_pid, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err); //TODO: Check for ENOENT (tester closed mq) thrown here!
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to open tester mq in async child"); // TODO: Instead of killing all we can probably get away with killing tester_pid iff completed==true, and failing silently otherwise

            tester_mq_send(tester_mq, word, completed, ignored, accepted, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to send to tester mq in async child");
            log_formatted("%d - SND: %s, COMPLETED=%d, IGNORED=%d, ACCEPTED=%d", getpid(), word, completed, ignored, accepted);

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to close tester mq in async child");
            // TODO: kill return (?)
            exit(EXIT_SUCCESS);

        default:
            await_forks++;
            await_runs--;
            break;
    }
}


int main() {
    assert(err == false);
    // initial setup
    main_pid = getpid();
    tester_list_t * tester_data = tester_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("Failed to create tester list");
    automaton a;
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

        tester_t *tester = tester_list_find(tester_data, validator_msg.tester_pid);
        if(!tester) {
            tester = tester_list_emplace(tester_data, validator_msg.tester_pid, 0, 0, 0, false, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to insert tester to list");
        }
        log_formatted("%d RCD: %s, completed=%d, finish=%d, for TESTER IN LIST: %d, bal=%d, completed=%d",
                      getpid(), validator_msg.word, validator_msg.completed, validator_msg.finish,
                      tester->pid, tester->word_bal, tester->completed);

        // process request
        assert(tester != NULL);
        if(validator_msg.start) {
            assert(!validator_msg.halt);

            // update logs
            comm_summary.rcd++;
            tester->rcd++;
            tester->word_bal++;
            tester->completed = validator_msg.completed;

            // prepare response TODO: Function (from here to end of case)
            validator_msg.start = false;
            validator_msg.finish = true;
            validator_msg.accepted = false;

            // perform run asynchronously
            int pipe_dsc[2];
            pipe(pipe_dsc); // TODO: Handle err

            async_create_run(pipe_dsc);
            close(pipe_dsc[0]); // TODO: Handle err

            async_pipe_data(pipe_dsc, &a, &validator_msg);
            close(pipe_dsc[1]); // TODO: Handle err
        } else if(validator_msg.halt) {
            assert(!validator_msg.start);
            assert(validator_msg.completed);

            // update logs
            halt_flag_raised = true;
            tester->completed = true;

            // send completed flag if necessary
            bool send_completed = (tester->word_bal == 0);
            if(send_completed) {
                async_send_to_tester(tester->pid, "THIS IS JUST A FLAG", send_completed, true, validator_msg.accepted);
                HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send completed flag to tester");
            }
        } else if(validator_msg.finish) {
            assert(!validator_msg.start);

            // update logs
            tester->word_bal--;
            if(validator_msg.accepted) {
                comm_summary.acc++;
                tester->acc++;
            }
            // format and send response
            bool send_completed = (tester->completed && tester->word_bal == 0);
            async_send_to_tester(tester->pid, validator_msg.word, send_completed, false, validator_msg.accepted);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send message to tester");
        } else {
            err = true;
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Unexpected type of request in validator mq");
        }
    }
    // after halt flag is raised, wait for runs to complete and ignore other testers if necessary
    while(await_runs) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to receive message from validator mq");

        tester_t *tester = tester_list_find(tester_data, validator_msg.tester_pid);
        if(!tester) {
            tester = tester_list_emplace(tester_data, validator_msg.tester_pid, 0, 0, 0, false, &err);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Failed to insert tester to list");
        }
        log_formatted("%d RCD: %s, completed=%d, finish=%d, for TESTER IN LIST: %d, bal=%d, completed=%d",
                      getpid(), validator_msg.word, validator_msg.completed, validator_msg.finish,
                      tester->pid, tester->word_bal, tester->completed);

        // process request
        assert(tester->word_bal >= 0);
        if(validator_msg.finish) {
            assert(!validator_msg.start);

            // update logs
            tester->word_bal--;
            if(validator_msg.accepted) {
                comm_summary.acc++;
                tester->acc++;
            }
            // format and send response
            bool send_completed = (tester->word_bal == 0); // different condition because of raised halt flag
            async_send_to_tester(tester->pid, validator_msg.word, send_completed, false, validator_msg.accepted);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to send response to tester asynchronously");
        } else {
            // no matter what it is, it should be ignored.
            bool send_completed = (tester->word_bal == 0); // different condition because of raised halt flag
            async_send_to_tester(tester->pid, validator_msg.word, send_completed, true, false);
            HANDLE_ERR_WITH_MSG(kill_all_exit, "Unable to ignore tester asynchronously");
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
