//
// Created by kowal on 16.01.18.
//

#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>
#include <wait.h>
#include <assert.h>
#include "../src/err.h"
#include "../src/tester_mq.h"

// TEST PARAMS:
const bool enable_log = true;
const int send_delay_in_seconds = 3;




#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"
void log(const char * msg) {
    if(enable_log) {
        fprintf(stderr, "%s\n", msg);
    }
}
#pragma clang diagnostic pop


void sender(const char * mq_name) {
    log("SENDER: creating mq...");
    mqd_t tester_mq = tester_mq_start(false, mq_name);

    log("SENDER: sending HALT...");
    tester_mq_send_halt(tester_mq);
    sleep(send_delay_in_seconds);

    log("SENDER: sending VALIDATION PASS...");
    tester_mq_send_validation_result(tester_mq, "dupa", "A");
    sleep(send_delay_in_seconds);

    log("SENDER: sending VALIDATION FAIL...");
    tester_mq_send_validation_result(tester_mq, "dupa", "N");
    sleep(send_delay_in_seconds);

    log("SENDER: finishing...");
    tester_mq_finish(false, tester_mq, NULL);
    log("SENDER: finished");
}

void async_sender(const char * mq_name) {
    switch(fork()) {
        case -1:
            syserr("TEST: Error in fork");
        case 0:
            // child
            sender(mq_name);
            exit(0);
        default:
            break;
    }
}

int main() {
    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(getpid(), tester_mq_name);

    printf("Starting fork with queue name: %s\n", tester_mq_name);
    async_sender(tester_mq_name);

    mqd_t tester_mq = tester_mq_start(true, tester_mq_name);

    struct mq_attr tmp;
    mq_getattr(tester_mq, &tmp);
    printf("%li\n", tmp.mq_msgsize);

    int i=3;
    size_t tester_mq_buffsize = tester_mq_get_buffsize(tester_mq);
    char tester_mq_buff[tester_mq_buffsize];
    ssize_t response_ret;
    while(--i >= 0) {
        response_ret = tester_mq_receive(tester_mq, tester_mq_buff, tester_mq_buffsize);
        if(response_ret == -1) {
            log("RECEIVER: Error receiving message");
        } else {
            switch (i){
                case 2:
                    assert(tester_mq_received_halt(tester_mq_buff, response_ret));
                    break;
                default:
                    assert(tester_mq_received_validation_result(tester_mq_buff, response_ret));
            }
            log("RECEIVER: received message:");
            log(tester_mq_buff);
        }
        log("");
    }

    log("RECEIVER: finishing...");
    tester_mq_finish(true, tester_mq, tester_mq_name);
    log("RECEIVER: finished");

    wait(NULL); // wait for sender
    return 0;
}
