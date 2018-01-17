//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_TESTER_MQ_H
#define PW_VALIDATOR_TESTER_MQ_H

#include <stdbool.h>
#include <mqueue.h>

#include "config.h"

#define TESTER_MQ_NAME_PREFIX_LEN 24
#define TESTER_MQ_NAME_LEN (TESTER_MQ_NAME_PREFIX_LEN + PID_STR_LEN)

// [word] [flag]\n\0
#define TESTER_MQ_BUFFSIZE (WORD_LEN_MAX + 4)

extern const char TESTER_MQ_FLAG_HALT;
extern const char TESTER_MQ_FLAG_PASSED;
extern const char TESTER_MQ_FLAG_FAILED;

extern const char TESTER_MQ_NAME_PREFIX[]; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly

/**
 * Creates C-string containing tester_mq name created based on provided pid
 * @param pid
 * @param target
 */
extern void tester_mq_get_name_from_pid(pid_t pid, char * target);

/**
 * Creates C-string containing tester_mq name, created based on provided pid string
 * @param pid_msg_part
 * @param target
 */
extern void tester_mq_get_name_from_pidstr(const char *pid_msg_part, char *target);

/**
 * Creates tester mq, for server read-only with initial setup, for client write-only.
 * @param server - if true, creates version for the server
 * @param tester_mq_name
 * @return - descriptor of created tester mq
 */
extern mqd_t tester_mq_start(bool server, const char * tester_mq_name);

/**
 * Returns buffer size necesssary to receive message from provided tester_mq
 * @param queue
 * @return - minimum necessary size of buffer
 */
size_t tester_mq_get_buffsize(mqd_t queue);

/**
 * Blocking.
 * Sends halt to provided tester mq
 * @param tester_mq
 */
extern void tester_mq_send_halt(mqd_t tester_mq);

/**
 * Blocking.
 * Sends validation result to provided tester mq
 * @param tester_mq
 * @param word - word that was validated
 * @param flag - validation result
 */
extern void tester_mq_send_validation_result(mqd_t tester_mq, const char * word, const char * flag);

/**
 * Blocking.
 * Receives message from provided tester mq and saves it into provided buffer
 * @param tester_mq
 * @param buffer
 * @param buffer_size - size of buffer, must not be smaller than tester_mq message size
 * @return - size of received message
 */
extern ssize_t tester_mq_receive(mqd_t tester_mq, char * buffer, size_t buffer_size);

/**
 * Checks if tester mq received halt message
 * @param buffer - buffer containing message
 * @param buffer_size - actual length of data in buffer
 * @return
 */
extern bool tester_mq_received_halt(const char * buffer, ssize_t buffer_size);

/**
 * Checks if tester mq received validation result message
 * @param buffer - buffer containing message
 * @param buffer_size - actual length of data in buffer
 * @return
 */
extern bool tester_mq_received_validation_result(const char * buffer, ssize_t buffer_size);

/**
 * Closes provided tester mq. In server version, frees tester mq name.
 * Make sure server version is executed after all clients are closed.
 * @param server - if true, performs server version of the function
 * @param tester_mq
 * @param tester_mq_name
 */
void tester_mq_finish(bool server, mqd_t tester_mq, const char * tester_mq_name);

#endif //PW_VALIDATOR_TESTER_MQ_H
