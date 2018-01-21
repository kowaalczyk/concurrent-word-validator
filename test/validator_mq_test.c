//
// Created by kowal on 16.01.18.
//
// TODO

#include <stdbool.h>
#include <stdio.h>
#include <zconf.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <mqueue.h>
#include <assert.h>
#include <wait.h>
#include "../src/validator_mq.h"


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


void sender() {
    bool err = false;

    test_log("SENDER: creating mq...");
    mqd_t validator_mq = validator_mq_start(false, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: created mq");

    test_log("SENDER: sending START...");
    validator_mq_send(validator_mq, true, false, 0, false, false, getpid(), "start", &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent START");
    sleep(send_delay_in_seconds);

    test_log("SENDER: sending HALT...");
    validator_mq_send(validator_mq, false, true, 0, false, false, getpid(), "halt", &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent HALT");
    sleep(send_delay_in_seconds);

    test_log("SENDER: sending FINISHED...");
    validator_mq_send(validator_mq, false, false, 0, true, false, getpid(), "finish", &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent FINISHED");
    sleep(send_delay_in_seconds);

    test_log("SENDER: sending ACCEPTED...");
    validator_mq_send(validator_mq, false, false, 0, false, true, getpid(), "accepted", &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: sent ACCEPTED");
    sleep(send_delay_in_seconds);

    test_log("SENDER: finishing...");
    validator_mq_finish(false, validator_mq, &err);
    HANDLE_ERR(err_sender);
    test_log("SENDER: finish");
}

pid_t async_sender() {
    pid_t pid;
    switch(pid = fork()) {
        case -1:
            test_log("Error in fork");
            exit(-1);
        case 0:
            // child
            sleep(10);
            sender();
            exit(0);
        default:
            return pid;
    }
}

// message order: start, halt, finish, accepted
int main() {
    bool err = false;

    printf("Starting fork\n");
    pid_t child_pid = async_sender();

//    sleep(10); // sender will fail if queue is not opened!
    test_log("RECEIVER: creating mq...");
    mqd_t validator_mq = validator_mq_start(true, &err);
    HANDLE_ERR(err_receiver);
    test_log("RECEIVER: created mq");

    int i=4;
    validator_mq_msg msg;
    while(--i >= 0) {
        validator_mq_receive(validator_mq, &msg, &err);
        HANDLE_ERR(err_receiver);
        switch (i){
            case 3:
                assert(msg.start);
                break;
            case 2:
                assert(msg.halt);
                break;
            case 1:
                assert(msg.finish);
                break;
            case 0:
                assert(msg.accepted);
                break;
            default:
                test_log("BAD MESSAGE");
                exit(-1);
                break;
        }
        assert(msg.tester_pid == child_pid);
        test_log("RECEIVER: received message:");
        test_log(msg.word);
        test_log("");
    }

    test_log("RECEIVER: finishing...");
    validator_mq_finish(true, validator_mq, &err);
    HANDLE_ERR(err_receiver);
    test_log("RECEIVER: finish");

    wait(NULL); // wait for sender
    return -errno;
}
