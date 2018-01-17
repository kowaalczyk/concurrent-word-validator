//
// Created by kowal on 13.01.18.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>

#include "tester_mq.h"
#include "err.h"

#define TESTER_MQ_PERMISSIONS 0666

const char TESTER_MQ_FLAG_HALT = HALT_FLAG;
const char TESTER_MQ_FLAG_PASSED = VALIDATION_PASSED_FLAG;
const char TESTER_MQ_FLAG_FAILED = VALIDATION_FAILED_FLAG;

const char TESTER_MQ_NAME_PREFIX[] = "/pw_validator_tester_mq_"; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly

void tester_mq_get_name_from_pid(pid_t pid, char *target) {
    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    pidstr(pid, target + TESTER_MQ_NAME_PREFIX_LEN);
}

void tester_mq_get_name_from_pidstr(const char *pid_msg_part, char *target) {
    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    memcpy(target+TESTER_MQ_NAME_PREFIX_LEN, pid_msg_part, PID_STR_LEN);
}

mqd_t tester_mq_start(bool server, const char *tester_mq_name) {
    mqd_t queue;
    if(server) {
        queue = mq_open(tester_mq_name, O_RDONLY | O_CREAT, TESTER_MQ_PERMISSIONS, NULL); // TODO: Handle possible errors
        struct mq_attr tester_mq_attrs;
        mq_getattr(queue, &tester_mq_attrs); // TODO: Handle possible errors

        if(tester_mq_attrs.mq_msgsize < TESTER_MQ_BUFFSIZE) {
        tester_mq_attrs.mq_msgsize = TESTER_MQ_BUFFSIZE;
            mq_setattr(queue, &tester_mq_attrs, NULL); // TODO: Handle possible errors
        }
    } else {
        queue = mq_open(tester_mq_name, O_WRONLY); // TODO: Handle possible errors
    }
    if(queue == -1) {
        syserr("Failed to create tester_mq"); // TODO: Better error handling
    }
    return queue;
}

size_t tester_mq_get_buffsize(mqd_t queue) {
    struct mq_attr tmp;
    mq_getattr(queue, &tmp); // TODO: Handle errors
    return (size_t) tmp.mq_msgsize;
}

void tester_mq_finish(bool server, mqd_t tester_mq, const char * tester_mq_name) {
    if (mq_close(tester_mq)) syserr("Failed to close queue"); // TODO: Better error handling

    if(server) {
        assert(tester_mq_name != NULL);
        if (mq_unlink(tester_mq_name)) syserr("Failed to unlink queue"); // TODO: Better error handling
    }
}

void tester_mq_send_halt(mqd_t tester_mq) {
    char buff[TESTER_MQ_BUFFSIZE];
    buff[0] = TESTER_MQ_FLAG_HALT;
    buff[1] = '\n';
    buff[2] = '\0';

    int ret = mq_send(tester_mq, buff, 2, HALT_FLAG_PRIORITY);
    if(ret == -1) {
        syserr("Error while trying to send halt to tester_mq"); // TODO: Better error handling
    }
}

extern void tester_mq_send_validation_result(mqd_t tester_mq, const char * word, const char * flag) {
    assert(flag[0] == TESTER_MQ_FLAG_PASSED || flag[0] == TESTER_MQ_FLAG_FAILED);

    char buff[TESTER_MQ_BUFFSIZE];
    size_t word_len = strlen(word);

    memcpy(buff, word, word_len);
    buff[word_len] = ' ';
    memcpy(buff+word_len+1, flag, 1);
    buff[word_len+2] = '\n';

    int ret = mq_send(tester_mq, buff, word_len+3, 1);
    if(ret == -1) {
        syserr("Error while trying to send word validation result to tester_mq"); // TODO: Better error handling
    }
}

ssize_t tester_mq_receive(mqd_t tester_mq, char *buffer, size_t buffer_size) {
    ssize_t request_ret = mq_receive(tester_mq, buffer, buffer_size, NULL);
    if(request_ret == -1) {
        syserr("Error while trying to receive message in tester_mq");
    }
    return request_ret;
}

bool tester_mq_received_halt(const char *buffer, ssize_t buffer_size) {
    printf("%zi %c\n", buffer_size, buffer[buffer_size-2]);
    return buffer[buffer_size-2] == '!';
}

bool tester_mq_received_validation_result(const char *buffer, ssize_t buffer_size) {
    printf("%zi %c\n", buffer_size, buffer[buffer_size-2]);
    return buffer[buffer_size-2] == TESTER_MQ_FLAG_PASSED || buffer[buffer_size-2] == TESTER_MQ_FLAG_FAILED;
}
