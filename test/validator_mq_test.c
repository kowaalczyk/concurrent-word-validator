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


void sender() {
    bool err = false;

    log("SENDER: creating mq...");
    mqd_t validator_mq = validator_mq_start(false, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: created mq");

    log("SENDER: sending START...");
    validator_mq_send(validator_mq, true, false, false, false, getpid(),"start", &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent START");
    sleep(send_delay_in_seconds);
    
    log("SENDER: sending HALT...");
    validator_mq_send(validator_mq, false, true, false, false, getpid(),"halt", &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent HALT");
    sleep(send_delay_in_seconds);

    log("SENDER: sending FINISHED...");
    validator_mq_send(validator_mq, false, false, true, false, getpid(),"finished", &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent FINISHED");
    sleep(send_delay_in_seconds);

    log("SENDER: sending ACCEPTED...");
    validator_mq_send(validator_mq, false, false, false, true, getpid(),"accepted", &err);
    HANDLE_ERR(err_sender);
    log("SENDER: sent ACCEPTED");
    sleep(send_delay_in_seconds);

    log("SENDER: finishing...");
    validator_mq_finish(false, validator_mq, &err);
    HANDLE_ERR(err_sender);
    log("SENDER: finished");
}

pid_t async_sender() {
    pid_t pid;
    switch(pid = fork()) {
        case -1:
            log("Error in fork");
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

// message order: start, halt, finished, accepted
int main() {
    bool err = false;

    printf("Starting fork\n");
    pid_t child_pid = async_sender();

//    sleep(10); // sender will fail if queue is not opened!
    log("RECEIVER: creating mq...");
    mqd_t validator_mq = validator_mq_start(true, &err);
    HANDLE_ERR(err_receiver);
    log("RECEIVER: created mq");

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
                assert(msg.finished);
                break;
            case 0:
                assert(msg.accepted);
                break;
            default:
                log("BAD MESSAGE");
                exit(-1);
                break;
        }
        assert(msg.tester_pid == child_pid);
        log("RECEIVER: received message:");
        log(msg.word);
        log("");
    }

    log("RECEIVER: finishing...");
    validator_mq_finish(true, validator_mq, &err);
    HANDLE_ERR(err_receiver);
    log("RECEIVER: finished");

    wait(NULL); // wait for sender
    return -errno;
}
