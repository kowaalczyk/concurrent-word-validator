//
// Created by kowal on 04.01.18.
//

#ifndef PW_VALIDATOR_REQUEST_QUEUE_H
#define PW_VALIDATOR_REQUEST_QUEUE_H


#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include "response_queue.h"

#define VALIDATION_START_FLAG '1'
#define VALIDATION_FINISH_FLAG '2'
#define VALIDATION_HALT_FLAG '3'
#define VALIDATION_REQUEST_SEPARATOR '-'
#define VALIDATION_PASSED_SYMBOL 'A'
#define VALIDATION_FAILED_SYMBOL 'N'

#define VALIDATOR_INCOMING_REQUESTS_MQ_NAME "automata_validator_server_validation_request_q"
#define VALIDATOR_INCOMING_REQUESTS_MQ_BUFFSIZE (STR_LEN_MAX + PID_FORMAT_LENGTH + 1 + 3)


/**
 * Request form: "[TYPE]-[PID]-[DATA]", where:
 * TYPE : {1, 2, 3} (1. is sent from tester, 2. from run, 3. sent from tester)
 * PID : (pid of a tester requesting the validation, in dec format filled with leading zeros to fit sizeof(pid_t))
 *
 */

/// checks if request type is HALT
bool requested_halt(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATION_HALT_FLAG;
}

/// checks if request type is FINISH
bool requested_validation_finish(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATION_FINISH_FLAG;
}

/// checks if request type is START
bool requested_validation_start(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == VALIDATION_START_FLAG;
}


void request__halt() {

}

void request_validation_finish(mqd_t request_q, const char * return_pid, const char * word, bool valid) {
    char buff[VALIDATOR_INCOMING_REQUESTS_MQ_BUFFSIZE];
    size_t word_len = strlen(word);

    // TODO: Test message read/write formatting
    buff[0] = VALIDATION_FINISH_FLAG;
    buff[1] = VALIDATION_REQUEST_SEPARATOR;
    memcpy(&buff[2], return_pid, PID_FORMAT_LENGTH);
    buff[PID_FORMAT_LENGTH+2] = VALIDATION_REQUEST_SEPARATOR;
    memcpy(&buff[PID_FORMAT_LENGTH+3], word, word_len);
    buff[PID_FORMAT_LENGTH+word_len+3] = ' ';
    if(valid) {
        buff[PID_FORMAT_LENGTH+word_len+4] = VALIDATION_PASSED_SYMBOL;
    } else {
        buff[PID_FORMAT_LENGTH+word_len+4] = VALIDATION_FAILED_SYMBOL;
    }
    buff[PID_FORMAT_LENGTH+word_len+5] = '\n';
    buff[PID_FORMAT_LENGTH+word_len+6] = '\0';

    int ret;
    ret = mq_send(request_q, buff, PID_FORMAT_LENGTH+word_len+7, 1);
    if(!ret) {
        syserr("RUN: Failed to send validation finish request");
    }
}

void request_validation_start() {

}



/// returns name of queue to which the response from run should be forwarded
char * get_response_queue_name(const char * buffer, ssize_t buffer_length) {
    char * ans = malloc(PID_FORMAT_LENGTH*sizeof(char));
    memcpy(ans, buffer, PID_FORMAT_LENGTH);
    return ans;
}


#endif //PW_VALIDATOR_REQUEST_QUEUE_H
