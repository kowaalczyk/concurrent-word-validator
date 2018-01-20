//
// Created by kowal on 16.01.18.
//

#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>
#include <wait.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <memory.h>
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

void err_receiver() {
    int n = errno;
    fprintf(stderr, "Error in receiver: %s\n", strerror(errno));
    exit(errno);
}
void err_sender() {
    int n = errno;
    fprintf(stderr, "Error in sender: %s\n", strerror(errno));
    exit(errno);
}


void sender(const char * mq_name) {
    bool err = false;

    log("SENDER: creating mq...");
    mqd_t tester_mq = tester_mq_start(false, mq_name, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: created mq");

    log("SENDER: sending COMPLETED...");
    tester_mq_send(tester_mq, "completed", true, false, false, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent COMPLETED");
    sleep(send_delay_in_seconds);

    log("SENDER: sending IGNORED...");
    tester_mq_send(tester_mq, "ignored", false, true, false, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent IGNORED");
    sleep(send_delay_in_seconds);

    log("SENDER: sending ACCEPTED...");
    tester_mq_send(tester_mq, "accepted", false, false, true, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent ACCEPTED");
    sleep(send_delay_in_seconds);

    log("SENDER: finishing...");
    tester_mq_finish(false, tester_mq, NULL, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: finished");
}

void async_sender(const char * mq_name) {
    switch(fork()) {
        case -1:
            log("Error in fork");
            exit(-1);
        case 0:
            // child
            sleep(10);
            sender(mq_name);
            exit(0);
        default:
            break;
    }
}

// msg order: completed, ignored, accepted (w/ word)
int main() {
    bool err = false;

    char tester_mq_name[TESTER_MQ_NAME_LEN];
    tester_mq_get_name_from_pid(getpid(), tester_mq_name);

    printf("Starting fork with queue name: %s\n", tester_mq_name);
    async_sender(tester_mq_name);

//    sleep(10); // sender will fail if queue is not opened!
    log("RECEIVER: creating mq...");
    mqd_t tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR(err_receiver);
    log("RECEIVER: created mq");

    int i=3;
    tester_mq_msg msg;
    while(--i >= 0) {
        tester_mq_receive(tester_mq, &msg, &err);
        HANDLE_ERR(err_receiver);
        switch (i){
            case 2:
                assert(msg.completed);
                break;
            case 1:
                assert(msg.ignored);
                break;
            case 0:
                assert(msg.accepted);
                break;
            default:
                log("BAD MESSAGE");
                exit(-1);
                break;
        }
        log("RECEIVER: received message:");
        log(msg.word);
        log("");
    }

    log("RECEIVER: finishing...");
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(err_receiver);
    log("RECEIVER: finished");

    wait(NULL); // wait for sender
    return -errno;
}
