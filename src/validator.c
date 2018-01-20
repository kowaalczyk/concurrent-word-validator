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
mqd_t validator_mq;

/**
 * Error handler.
 * Raises halt flag, without exiting.
 */
void raise_halt_flag() {
    halt_flag_raised = true;
}

/**
 * Error handler.
 * Kills all known processes and exits the current process.
 */
void kill_all_exit() {
    // TODO
}

// TODO: Complete fix
void async_create_run(const validator_mq_msg request_msg) {
    assert(!halt_flag_raised);

    bool err = false;
    switch (fork()) {
        case -1:
            err = true;
            HANDLE_ERR(raise_halt_flag);
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

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

void async_pipe_data(const validator_mq_msg request_msg) {
    assert(!halt_flag_raised);

    bool err = false;
}

// TODO: Send complete to tester iff halt flag raised and tester.word_bal is 0

void async_forward_response(const tester_t *tester, validator_mq_msg request_msg) {
    bool err = false;
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    bool completed = (halt_flag_raised && tester->word_bal == 0);
    switch (fork()) {
        case -1:
            // TODO: Error handling
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_get_name_from_pid(tester->pid, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_send(tester_mq, request_msg.word, completed, false, request_msg.accepted, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(kill_all_exit);
            exit(0);
        default:
            await_forks++;
            await_runs--;
            break;
    }
}

void async_reply_ignore(const tester_t *tester, validator_mq_msg request_msg) {
    bool err = false;
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    mqd_t tester_mq;
    bool completed = (halt_flag_raised && tester->word_bal == 0);
    switch (fork()) {
        case -1:
            break;
        case 0:
            validator_mq_finish(false, validator_mq, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_get_name_from_pid(tester->pid, tester_mq_name);
            tester_mq = tester_mq_start(false, tester_mq_name, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_send(tester_mq, request_msg.word, completed, true, false, &err);
            HANDLE_ERR(kill_all_exit);

            tester_mq_finish(false, tester_mq, NULL, &err);
            HANDLE_ERR(kill_all_exit);
            exit(0);
        default:
            await_forks++;
            break;
    }
}


int main() {
    // initial setup
    bool err = false;
    comm_sumary_t comm_summary = {0, 0, 0};
    tester_list_t * tester_data = tester_list_create(&err);
    HANDLE_ERR_EXIT_WITH_MSG("VALIDATOR: Could not create tester list");

    // setup queues
    const automaton * a = load_automaton();
    validator_mq = validator_mq_start(true, &err);
    HANDLE_ERR_EXIT_WITH_MSG("VALIDATOR: Could not start validator mq");

    // handle incoming requests
    validator_mq_msg validator_msg;
    while(!halt_flag_raised) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR(raise_halt_flag);

        // process request
        comm_summary.rcd++;
        if(validator_msg.halt) {
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(tester) {
                tester->rcd++;
            } else {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 1, 0, 0, &err);
                HANDLE_ERR(raise_halt_flag);
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
            async_forward_response(tester, validator_msg);

        } else if(validator_msg.start) {
            // Update local logs and start validation TODO: Function
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(tester) {
                tester->word_bal++;
                tester->rcd++;
            } else {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 1, 0, 1, &err);
                HANDLE_ERR(raise_halt_flag);
            }
            async_create_run(validator_msg);

        } else {
            // TODO: Consider handling this
        }
    }
    // after halt flag is raised, wait for runs to complete and halt other testers if necessary
    while(await_runs) {
        validator_mq_receive(validator_mq, &validator_msg, &err);
        HANDLE_ERR_DECREMENT_CONTINUE(await_runs);

        // process request
        comm_summary.rcd++;
        if(validator_msg.finished) {
            // Update local logs and forward response TODO: Function
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            assert(tester != NULL);
            tester->word_bal--;
            if(validator_msg.accepted) {
                comm_summary.acc++;
                tester->acc++;
            }
            async_forward_response(tester, validator_msg);

        } else {
            // only happens when one tester send halt and other tester completed request before receiving signal
            tester_t * tester = tester_list_find(tester_data, validator_msg.tester_pid);
            if(tester) {
                tester->rcd += 1;
            } else {
                tester_list_emplace(tester_data, validator_msg.tester_pid, 1, 0, 0, &err);
                HANDLE_ERR(raise_halt_flag);
            }
            async_reply_ignore(tester, validator_msg);
        }
    }

    // clean up
    validator_mq_finish(true, validator_mq, &err);
    HANDLE_ERR(kill_all_exit);
    free((void *) a);
    while(await_forks) {
        pid_t tmp_pid;
        int wait_ret;
        tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1) {
            kill_all_exit();
        }
        if(wait_ret == 0) {
            comm_summary.snt++;
        }
        await_forks--;
    }
    print_comm_summary(&comm_summary);
    tester_list_print_log(tester_data);
    tester_list_destroy(tester_data);
    return 0;
}
