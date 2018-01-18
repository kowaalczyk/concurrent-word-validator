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

static const char TESTER_MQ_FLAG_HALT = HALT_FLAG;
static const char TESTER_MQ_FLAG_PASSED = VALIDATION_PASSED_FLAG;
static const char TESTER_MQ_FLAG_FAILED = VALIDATION_FAILED_FLAG;

static const char TESTER_MQ_NAME_PREFIX[] = "/pw_validator_tester_mq_"; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly

void tester_mq_get_name_from_pid(pid_t pid, char *target) {
    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    pidstr(pid, target + TESTER_MQ_NAME_PREFIX_LEN);
}

void tester_mq_get_name_from_pidstr(const char *pid_msg_part, char *target) {
    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    memcpy(target+TESTER_MQ_NAME_PREFIX_LEN, pid_msg_part, PID_STR_LEN);
}

mqd_t tester_mq_start(bool server, const char *tester_mq_name, bool *err) {
    int tmp_err;
    mqd_t queue;

    if(server) {
        queue = mq_open(tester_mq_name, O_RDONLY | O_CREAT, TESTER_MQ_PERMISSIONS, NULL);
        INT_FAIL_IF(queue == -1);

        // resize queue message size if it is too small
        struct mq_attr tester_mq_attrs;
        tmp_err = mq_getattr(queue, &tester_mq_attrs);
        INT_FAIL_IF(tmp_err == -1);
        if(tester_mq_attrs.mq_msgsize < TESTER_MQ_BUFFSIZE) {
            tester_mq_attrs.mq_msgsize = TESTER_MQ_BUFFSIZE;
            tmp_err = mq_setattr(queue, &tester_mq_attrs, NULL); // TODO: This might be impossible, instead of resizing set correct errno
            INT_FAIL_IF(tmp_err == -1);
        }
    } else {
        queue = mq_open(tester_mq_name, O_WRONLY);
        INT_FAIL_IF(queue == -1);
    }
    assert(queue != -1);
    return queue;
}

size_t tester_mq_get_buffsize(mqd_t queue, bool *err) {
    int tmp_err;
    struct mq_attr tmp;

    tmp_err = mq_getattr(queue, &tmp);
    INT_FAIL_IF(tmp_err == -1);
    return (size_t) tmp.mq_msgsize;
}

void tester_mq_finish(bool server, mqd_t tester_mq, const char *tester_mq_name, bool *err) {
    int tmp_err;

    tmp_err = mq_close(tester_mq);
    VOID_FAIL_IF(tmp_err == -1);

    if(server) {
        tmp_err = mq_unlink(tester_mq_name);
        VOID_FAIL_IF(tmp_err == -1);
    }
}

void tester_mq_send_halt(mqd_t tester_mq, bool *err) {
    int tmp_err;
    char buff[TESTER_MQ_BUFFSIZE];

    buff[0] = TESTER_MQ_FLAG_HALT;
    buff[1] = '\n';
    buff[2] = '\0';

    tmp_err = mq_send(tester_mq, buff, 2, HALT_FLAG_PRIORITY);
    VOID_FAIL_IF(tmp_err == -1);
}

extern void tester_mq_send_validation_result(mqd_t tester_mq, const char *word, const char *flag, bool *err) {
    assert(flag[0] == TESTER_MQ_FLAG_PASSED || flag[0] == TESTER_MQ_FLAG_FAILED);

    int tmp_err;
    char buff[TESTER_MQ_BUFFSIZE];
    size_t word_len = strlen(word);

    memcpy(buff, word, word_len);
    buff[word_len] = ' ';
    memcpy(buff+word_len+1, flag, 1);
    buff[word_len+2] = '\n';
    buff[word_len+3] = '\0';

    tmp_err = mq_send(tester_mq, buff, word_len+3, 1);
    VOID_FAIL_IF(tmp_err == -1);
}

ssize_t tester_mq_receive(mqd_t tester_mq, char *buffer, size_t buffer_size, bool *err) {
    ssize_t request_ret = mq_receive(tester_mq, buffer, buffer_size, NULL);
    INT_FAIL_IF(request_ret == 0);
    return request_ret;
}

bool tester_mq_received_halt(const char *buffer, ssize_t buffer_size) {
    return buffer[buffer_size-2] == '!';
}

bool tester_mq_received_validation_result(const char *buffer, ssize_t buffer_size) {
    return buffer[buffer_size-2] == TESTER_MQ_FLAG_PASSED || buffer[buffer_size-2] == TESTER_MQ_FLAG_FAILED;
}
