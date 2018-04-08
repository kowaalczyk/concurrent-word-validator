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


void test_log(const char *msg) {
    if(enable_log) {
        fprintf(stderr, "%s\n", msg);
    }
}

void err_receiver() {
    int n = errno;
    fprintf(stderr, "Error in receiver: %s\n", strerror(n));
    exit(n);
}
void err_sender() {
    int n = errno;
    fprintf(stderr, "Error in sender: %s\n", strerror(n));
    exit(n);
}


void sender(const char * mq_name) {
    bool err = false;

    test_log("SENDER: creating mq...");
    mqd_t tester_mq = tester_mq_start(false, mq_name, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: created mq");

    test_log("SENDER: sending COMPLETED...");
    tester_mq_send(tester_mq, "completed", true, 0, false, false, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent COMPLETED");
    sleep(send_delay_in_seconds);

    test_log("SENDER: sending IGNORED...");
    tester_mq_send(tester_mq, "ignored", false, 0, true, false, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent IGNORED");
    sleep(send_delay_in_seconds);

    test_log("SENDER: sending ACCEPTED...");
    tester_mq_send(tester_mq, "accepted", false, 0, false, true, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent ACCEPTED");
    sleep(send_delay_in_seconds);

    test_log("SENDER: finishing...");
    tester_mq_finish(false, tester_mq, NULL, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: finish");
}

void async_sender(const char * mq_name) {
    switch(fork()) {
        case -1:
            test_log("Error in fork");
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
    test_log("RECEIVER: creating mq...");
    mqd_t tester_mq = tester_mq_start(true, tester_mq_name, &err);
    HANDLE_ERR(err_receiver);
    test_log("RECEIVER: created mq");

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
                test_log("BAD MESSAGE");
                exit(-1);
                break;
        }
        test_log("RECEIVER: received message:");
        test_log(msg.word);
        test_log("");
    }

    test_log("RECEIVER: finishing...");
    tester_mq_finish(true, tester_mq, tester_mq_name, &err);
    HANDLE_ERR(err_receiver);
    test_log("RECEIVER: finish");

    wait(NULL); // wait for sender
    return -errno;
}
