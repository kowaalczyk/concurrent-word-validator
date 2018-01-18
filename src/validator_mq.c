//
// Created by kowal on 13.01.18.
//

#include "validator_mq.h"
#include "err.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// TODO: Figure a way to kill everything in case of error

const char VALIDATOR_MQ_FLAG_HALT = HALT_FLAG;
const char VALIDATOR_MQ_FLAG_START = 'S';
const char VALIDATOR_MQ_FLAG_FINISH_PASSED = VALIDATION_PASSED_FLAG;
const char VALIDATOR_MQ_FLAG_FINISH_FAILED = VALIDATION_FAILED_FLAG;


mqd_t validator_mq_start(bool server, bool *err) {
    // TODO: Server
//    char * q_name = PW_VQ_REQUEST_NAME_PREFIX;
//    mqd_t incoming_request_q = mq_open(q_name, O_RDONLY | O_CREAT);
//    if(incoming_request_q == -1) {
//        syserr("VALIDATOR: Failed to create requests queue");
//    }

    // TODO: Client
//    char * validator_mq_name = PW_VQ_REQUEST_NAME_PREFIX;
//    mqd_t validator_mq = mq_open(validator_mq_name, O_WRONLY);
//    if(validator_mq == -1) {
//        syserr("TESTER: Failed to create requests queue");
//    }
    return 0;
}

void validator_mq_send_validation_start_request(mqd_t validator_mq, const char *word, bool *err) {
    // TODO
//    mq_send(validator_mq, word, strlen(word), 1);
}

ssize_t validator_mq_receive(mqd_t validator_mq, char *buffer, size_t buffer_size, bool *err) {
    // TODO
//    ssize_t request_ret = mq_receive(validator_mq, buffer, buffer_size, NULL);
//    if(request_ret < 0) {
//        syserr("VALIDATOR: Failed to receive request");
//    }
    return 0;
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

void validator_mq_finish(mqd_t validator_mq, bool *err) {
    // TODO
//    if (mq_close(incoming_request_q)) syserr("VALIDATOR: Failed to close queue");
//    if (mq_unlink(q_name)) syserr("VALIDATOR: Failed to unlink queue");
}

void validator_mq_extract_pidstr(const char *buffer, char *target) {
    assert(sizeof(target) >= PID_STR_LEN);

    memcpy(target, buffer+2, PID_STR_LEN);
}

void validator_mq_extract_word(const char *buffer, char *target) {
    assert(sizeof(target) >= WORD_LEN_MAX);

    memcpy(target, buffer+3+PID_STR_LEN, WORD_LEN_MAX);
}

void validator_mq_extract_flag(const char *buffer, char *target) {
    assert(target != NULL);

    memcpy(target, buffer, 1);
}

