//
// Created by kowal on 21.01.18.
//

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <zconf.h>
#include <wait.h>
#include <assert.h>
#include <errno.h>
#include "../src/config.h"

// Adding sleep delay makes wait in parent break (because sleep is signal-implemented, i guess) ~50% of times.
//#define DELAY 3
#define N_FORKS 1000
#define NOTIFY_SIG (SIGRTMIN+1)

pid_t main_pid = 0;
struct sigaction action;
sigset_t block_mask;
size_t await_forks  = 0;

comm_sumary_t comm_sumary = {0, 0, 0};


void catch_async_return(int sig) {
    if(sig != NOTIFY_SIG) {
        log_formatted("Bad sig!");
        exit(EXIT_FAILURE);
    }
    if(main_pid != getpid()) {
        log_formatted("Bad pid: %d instead of %d!", getpid(), main_pid);
        exit(EXIT_FAILURE);
    }

    comm_sumary.acc++;
}


void long_async_func(int report_val) {
    int tmp_err = 0;

    switch (fork()) {
        case -1:
            log_formatted("Cannot fork!");
            exit(EXIT_FAILURE);
        case 0:
            close(0);
            close(1);
            tmp_err = sigprocmask(SIG_UNBLOCK, &block_mask, 0);
            if(tmp_err == -1) {
                log_formatted("Failed to restore sig handlers!");
            }
//            sleep(DELAY);

            // signal parent if necessary
            if(report_val) {
                tmp_err = kill(main_pid, NOTIFY_SIG);
                if(tmp_err == -1) {
                    log_formatted("Cannot send signal to parent %d: %d, %s", main_pid, errno, strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            exit(EXIT_SUCCESS);
        default:
            await_forks++;
    }
}

int main() {
    int tmp_err = 0;
    main_pid = getpid();
    log_formatted("Parent pid is %d", main_pid);

    // setup signal handlers
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = catch_async_return;

    tmp_err = sigaction(NOTIFY_SIG, &action, NULL);
    if(tmp_err == -1) {
        log_formatted("Error in setup");
        exit(EXIT_FAILURE);
    }

    // create workers
    for(int i=0; i<N_FORKS; i++) {
        long_async_func(i%2);
    }

    // wait for workers to finish
    while (await_forks) {
        int wait_ret = EXIT_SUCCESS;
        pid_t tmp_pid = wait(&wait_ret);
        if(tmp_pid == -1 || wait_ret != EXIT_SUCCESS) {
            log_formatted("Parent received failure: %d, %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        await_forks--;
    }
    log_formatted("Parent received # of returned values: %d\n", comm_sumary.acc);
    assert(comm_sumary.acc == N_FORKS/2);
    return 0;
}
