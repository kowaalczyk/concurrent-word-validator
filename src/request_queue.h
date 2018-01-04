//
// Created by kowal on 04.01.18.
//

#ifndef PW_VALIDATOR_REQUEST_QUEUE_H
#define PW_VALIDATOR_REQUEST_QUEUE_H


#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define VALIDATION_START_FLAG '1'
#define VALIDATION_FINISH_FLAG '2'
#define VALIDATION_HALT_FLAG '3'
#define VALIDATOR_INCOMING_REQUESTS_MQ_NAME "automata_validator_server_validation_request_q"
#define VALIDATOR_INCOMING_REQUESTS_MQ_BUFFSIZE (STR_LEN_MAX + sizeof(pid_t) + 1 + 3)


/**
 * Request form: "[TYPE]-[PID]-[DATA]", where:
 * TYPE : {1, 2, 3} (1. is sent from tester, 2. from run, 3. sent from tester)
 * PID : (pid of a tester requesting the validation, in binary format)
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


#endif //PW_VALIDATOR_REQUEST_QUEUE_H
