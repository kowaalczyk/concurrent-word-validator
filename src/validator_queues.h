//
// Created by kowal on 04.01.18.
//

#ifndef PW_VALIDATOR_VALIDATOR_QUEUES_H
#define PW_VALIDATOR_VALIDATOR_QUEUES_H


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

// helper parameters ---------------------------------------------------------------------------------------------------

#define PID_FORMAT_LENGTH (sizeof(pid_t))

// queue parameters ----------------------------------------------------------------------------------------------------
/**
 * Queue naming convention: "/PW_VALIDATOR_[REQUEST|RESPONSE]_[pid of opener]"
 *
 * REQUEST QUEUE:
 * One request queue per each validator.
 * Each validator receives messages only through its request queue.
 * Validator opens its request queue only for reading.
 * Testers and runs only open request queue for writing.
 * Request queue is created and destroyed by validator.
 *
 * RESPONSE QUEUE:
 * One response queue per tester.
 * Each tester receives messages only through its response queue.
 * Tester opens its response queue only for reading.
 * Validator can open multiple response queues (one per each unique tester sending request).
 * Runs do not have access to response queues.
 * Response queue is created and destroyed by tester.
 */

#define PW_VQ_REQUEST_NAME_PREFIX "/PW_VALIDATOR_REQUEST_"
#define PW_VQ_REQUEST_NAME_LENGTH (22+PID_FORMAT_LENGTH)
#define PW_VQ_REQUEST_BUFFSIZE (STR_LEN_MAX + PID_FORMAT_LENGTH + 4)

#define PW_VQ_RESPONSE_NAME_PREFIX "/PW_VALIDATOR_RESPONSE_"
#define PW_VQ_RESPONSE_NAME_LENGTH (23+PID_FORMAT_LENGTH)
#define PW_VQ_RESPONSE_BUFFSIZE (STR_LEN_MAX + 3)

// queue content parameters --------------------------------------------------------------------------------------------
/**
 * Request form: "[TYPE]-[TESTER_PID]-[DATA]\0", where:
 * TYPE : {1, 2, 3} (1. is sent from tester, 2. from run, 3. sent from tester)
 * TESTER_PID: ("%0*d", sizeof(pid_t), pid)
 * DATA:
 *   1 => [word]
 *   2 => [word A|N]
 *   3 => [NULL]
 *
 * Response form: "[word A|N]\n"
 */

#define PW_VQ_SEPARATOR '-'

#define PW_VQ_FLAG_START '1'
#define PW_VQ_FLAG_FINISH '2'
#define PW_VQ_FLAG_HALT '3'

#define PW_VQ_SYMBOL_PASSED 'A'
#define PW_VQ_SYMBOL_FAILED 'N'

#define PW_VQ_STR_FINISH "\n\0"

// TODO: Move all function definitions int .c file

// request helpers -----------------------------------------------------------------------------------------------------



// request checkers ----------------------------------------------------------------------------------------------------

/// checks if request type is HALT
bool requested_halt(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == PW_VQ_FLAG_HALT;
}

/// checks if request type is FINISH
bool requested_validation_finish(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == PW_VQ_FLAG_FINISH;
}

/// checks if request type is START
bool requested_validation_start(const char * buffer, ssize_t buffer_length) {
    assert(buffer_length>0);

    return buffer[0] == PW_VQ_FLAG_START;
}

// request creators ----------------------------------------------------------------------------------------------------
// TODO: Change to functions creating a request, not sending it (problems with closed / failed queues and blocking threads)

/// generates halt request and sends it into provided mq
void tester_request__halt() {
    // TODO
}

/// generates validation finish request and sends it into provided mq
void run_request_validation_finish(mqd_t request_q, const char * return_pid, const char * word, bool valid) {
    // TODO: Refactor
    char buff[PW_VQ_REQUEST_BUFFSIZE];
    size_t word_len = strlen(word);

    // TODO: Test message read/write formatting
    buff[0] = PW_VQ_FLAG_FINISH;
    buff[1] = PW_VQ_SEPARATOR;
    memcpy(&buff[2], return_pid, PID_FORMAT_LENGTH);
    buff[PID_FORMAT_LENGTH+2] = PW_VQ_SEPARATOR;
    memcpy(&buff[PID_FORMAT_LENGTH+3], word, word_len);
    buff[PID_FORMAT_LENGTH+word_len+3] = ' ';
    if(valid) {
        buff[PID_FORMAT_LENGTH+word_len+4] = PW_VQ_SYMBOL_PASSED;
    } else {
        buff[PID_FORMAT_LENGTH+word_len+4] = PW_VQ_SYMBOL_FAILED;
    }
    buff[PID_FORMAT_LENGTH+word_len+5] = '\n';
    buff[PID_FORMAT_LENGTH+word_len+6] = '\0';

    int ret;
    ret = mq_send(request_q, buff, PID_FORMAT_LENGTH+word_len+7, 1);
    if(!ret) {
        syserr("RUN: Failed to send validation finish request");
    }
}

/// generates validation start requests and sends it into provided mq
void tester_request_validation_start() {
    // TODO
}

// response helpers ----------------------------------------------------------------------------------------------------

/// extracts pid from a given requests and returns response queue name associated with it
char * get_response_queue_name(const char * request, ssize_t request_length) {
    // TODO: Refactor
    char * ans = malloc(PID_FORMAT_LENGTH*sizeof(char));
    memcpy(ans, request, PID_FORMAT_LENGTH);
    return ans;
}

/// creates appropriate name for response queue for a tester with given pid
char * create_response_queue_name(pid_t pid) {
    // TODO: Refactor
    char *ans = malloc(PW_VQ_RESPONSE_NAME_LENGTH*sizeof(char));
    sprintf(ans, "%0*d%s", (int)sizeof(pid_t), pid, PW_VQ_RESPONSE_NAME_PREFIX);
    return ans;
}

// response creators ---------------------------------------------------------------------------------------------------

/// creates a response based on provided validation finish request
char * validator_create_response(const char * validation_finish_request, ssize_t request_length) {
    // TODO
}

// no response checkers / handlers are necessary

#endif //PW_VALIDATOR_VALIDATOR_QUEUES_H
