//
// Created by kowal on 13.01.18.
//

#include <string.h>
#include <mqueue.h>
#include <assert.h>
#include <stdio.h>
#include "tester_mq.h"
#include "err.h"


const char TESTER_MQ_FLAG_HALT = HALT_FLAG;
const char TESTER_MQ_FLAG_PASSED = VALIDATION_PASSED_FLAG;
const char TESTER_MQ_FLAG_FAILED = VALIDATION_FAILED_FLAG;

const char TESTER_MQ_NAME_PREFIX[] = "/pw_validator_tester_mq_"; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly


void tester_mq_get_name_from_pid(pid_t pid, char *target) {
    assert(sizeof(target) == TESTER_MQ_NAME_LEN);

    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    pidstr(pid, target + TESTER_MQ_NAME_PREFIX_LEN);
}

void tester_mq_get_name_from_pidstr(const char *pid_msg_part, char *target) {
    assert(sizeof(target) == TESTER_MQ_NAME_LEN);

    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    memcpy(target+TESTER_MQ_NAME_PREFIX_LEN, pid_msg_part, PID_STR_LEN);
    // TODO: Make sure no additional trailing \0 is needed
}

mqd_t tester_mq_start(bool server, const char *tester_mq_name) {
    mqd_t queue;
    if(server) {
        queue = mq_open(tester_mq_name, O_RDONLY | O_CREAT);
    } else {
        queue = mq_open(tester_mq_name, O_WRONLY);
    }
    if(queue == -1) {
        syserr("Failed to create tester_mq"); // TODO: Better error handling
    }
    return queue;
}

void tester_mq_finish(bool server, mqd_t tester_mq, const char * tester_mq_name) {
    if (mq_close(tester_mq)) syserr("Failed to close queue");

    if(server) {
        assert(tester_mq_name != NULL);
        if (mq_unlink(tester_mq_name)) syserr("Failed to unlink queue"); // TODO: Better error handling
    }
}

void tester_mq_send_halt(mqd_t tester_mq) {
    char buff[TESTER_MQ_BUFFSIZE];
    buff[0] = TESTER_MQ_FLAG_HALT;

    int ret = mq_send(tester_mq, buff, 1, HALT_FLAG_PRIORITY);
    if(!ret) {
        syserr("Error while trying to send halt to tester_mq"); // TODO: Better error handling
    }
}

extern void tester_mq_send_validation_result(mqd_t tester_mq, const char * word, const char * flag) {
    assert(sizeof(flag) == 1);
    assert(flag[0] == TESTER_MQ_FLAG_PASSED || flag[0] == TESTER_MQ_FLAG_FAILED);

    char buff[TESTER_MQ_BUFFSIZE];
    size_t word_len = strlen(word);

    memcpy(buff, word, word_len);
    buff[word_len] = ' ';
    memcpy(buff+word_len+1, flag, 1);
    buff[word_len+2] = '\n';

    int ret = mq_send(tester_mq, buff, word_len+3, 1);
    if(!ret) {
        syserr("Error while trying to send word validation result to tester_mq"); // TODO: Better error handling
    }
}

bool tester_mq_received_halt(const char *buffer, ssize_t buffer_size) {
    assert(buffer_size > 0);

    return buffer[buffer_size-1] == '!';
}

bool tester_mq_received_validation_result(const char *buffer, ssize_t buffer_size) {
    assert(buffer_size > 0);

    return buffer[buffer_size-1] == TESTER_MQ_FLAG_PASSED || buffer[buffer_size-1] == TESTER_MQ_FLAG_FAILED;
}
