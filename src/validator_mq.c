//
// Created by kowal on 13.01.18.
//

#include "validator_mq.h"
#include "err.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// TODO: Figure a way to kill everything in case of error

#define VALIDATOR_MQ_PERMISSIONS 0666

static const char VALIDATOR_MQ_NAME = "/pw_validator_validator_mq_single";

static const char VALIDATOR_MQ_FLAG_HALT = HALT_FLAG;
static const char VALIDATOR_MQ_FLAG_START = 'S';
static const char VALIDATOR_MQ_FLAG_FINISH_PASSED = VALIDATION_PASSED_FLAG;
static const char VALIDATOR_MQ_FLAG_FINISH_FAILED = VALIDATION_FAILED_FLAG;


mqd_t validator_mq_start(bool server, bool *err) {
    int tmp_err;
    mqd_t queue;

    if(server) {
        queue = mq_open(&VALIDATOR_MQ_NAME, O_RDONLY | O_CREAT, VALIDATOR_MQ_PERMISSIONS, NULL);
        INT_FAIL_IF(queue == -1);

        // resize queue message size if it is too small
        struct mq_attr tester_mq_attrs;
        tmp_err = mq_getattr(queue, &tester_mq_attrs);
        INT_FAIL_IF(tmp_err == -1);
        if(tester_mq_attrs.mq_msgsize < VALIDATOR_MQ_BUFFSIZE) {
            tester_mq_attrs.mq_msgsize = VALIDATOR_MQ_BUFFSIZE;
            tmp_err = mq_setattr(queue, &tester_mq_attrs, NULL); // TODO: This might be impossible, instead of resizing set correct errno
            INT_FAIL_IF(tmp_err == -1);
        }
    } else {
        queue = mq_open(&VALIDATOR_MQ_NAME, O_WRONLY);
        INT_FAIL_IF(queue == -1);
    }
    assert(queue != -1);
    return queue;
}

void validator_mq_send_validation_start_request(mqd_t validator_mq, const char *word, bool *err) {
    // TODO + re-check extracts
//    mq_send(validator_mq, word, strlen(word), 1);
}

ssize_t validator_mq_receive(mqd_t validator_mq, char *buffer, size_t buffer_size, bool *err) {
    ssize_t request_ret = mq_receive(validator_mq, buffer, buffer_size, NULL);
    INT_FAIL_IF(request_ret == 0);
    return request_ret;
}

bool validator_mq_requested_halt(const char *buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATOR_MQ_FLAG_HALT;
}

bool validator_mq_requested_validation_finish(const char *buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATOR_MQ_FLAG_FINISH_PASSED || buffer[0] == VALIDATOR_MQ_FLAG_FINISH_FAILED;
}

bool validator_mq_requested_validation_start(const char *buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATOR_MQ_FLAG_START;
}

void validator_mq_finish(mqd_t validator_mq, bool server, bool *err) {
    int tmp_err;

    tmp_err = mq_close(validator_mq);
    VOID_FAIL_IF(tmp_err == -1);

    if(server) {
        tmp_err = mq_unlink(&VALIDATOR_MQ_NAME);
        VOID_FAIL_IF(tmp_err == -1);
    }
}

void validator_mq_extract_pidstr(const char *buffer, char *target) {
    memcpy(target, buffer+2, PID_STR_LEN);
}

void validator_mq_extract_word(const char *buffer, char *target) {
    memcpy(target, buffer+3+PID_STR_LEN, WORD_LEN_MAX);
}

void validator_mq_extract_flag(const char *buffer, char *target) {
    memcpy(target, buffer, 1);
}

