//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_VALIDATOR_MQ_H
#define PW_VALIDATOR_VALIDATOR_MQ_H

#include <mqueue.h>
#include <stdbool.h>
#include "config.h"

// [flag]-[pidstr]-[word]\n\0
#define VALIDATOR_MQ_BUFFSIZE (WORD_LEN_MAX + PID_STR_LEN + 4)


/**
 * Creates message queue for sending requests to validator,
 * if any error occurs during creation, closes all programs.
 * @param server - if true, queue will be read-only
 * @return descriptor of created queue
 */
extern mqd_t validator_mq_start(bool server, bool *err);

/**
 * Returns buffer size necesssary to receive message from provided tester_mq
 * @param queue
 * @return - minimum necessary size of buffer
 */
extern size_t validator_mq_get_buffsize(mqd_t validator_mq, bool *err);

/**
 * Sends a validation start request containing provided word to provided mq,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq - message queue created using validator_mq_start
 * @param word - word to validate
 */
extern void validator_mq_send_validation_start_request(mqd_t validator_mq, const char *word, bool *err);

extern void validator_mq_send_validation_finish_request(mqd_t validator_mq, const char *word, char result_flag, bool *err);

extern void validator_mq_send_halt_request(mqd_t validator_mq, bool *err);

/**
 * Waits until a message is received via provided mq,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq - message queue created using validator_mq_start
 * @param buffer - buffer to which incoming message will be written
 * @param buffer_size - maximum length of message that can be written to buffer
 * @return size of received message
 */
extern ssize_t validator_mq_receive(mqd_t validator_mq, char *buffer, size_t buffer_size, bool *err);

/**
 * Checks if request type is halt.
 * @param buffer - buffer containing request
 * @param buffer_size - actual length of data in buffer
 * @return
 */
extern bool validator_mq_requested_halt(const char * buffer, ssize_t buffer_size);

/**
 * Checks if request type is start validation.
 * @param buffer - buffer containing request
 * @param buffer_size - actual length of data in buffer
 * @return
 */
extern bool validator_mq_requested_validation_start(const char * buffer, ssize_t buffer_size);

/**
 * Checks if request type is finish validation.
 * @param buffer - buffer containing request
 * @param buffer_size - actual length of data in buffer
 * @return
 */
extern bool validator_mq_requested_validation_finish(const char * buffer, ssize_t buffer_size);

/**
 * Extracts pidstr from buffer to target
 * @param buffer
 * @param target
 */
extern void validator_mq_extract_pidstr(const char * buffer, char * target);

extern pid_t validator_mq_extract_pid(const char *buffer);

/**
 * Extracts word from buffer to target
 * @param buffer
 * @param target
 */
extern void validator_mq_extract_word(const char * buffer, char * target);

/**
 * Extracts flag from buffer to target
 * @param buffer
 * @param target
 */
extern void validator_mq_extract_flag(const char * buffer, char * target);

/**
 * Returns result of the validation, if buffer contains validation finished request.
 * @param buffer
 * @return
 */
extern bool validator_mq_validation_passed(const char *buffer);

/**
 * Closes provided queue and cleans its data,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq
 */
extern void validator_mq_finish(mqd_t validator_mq, bool server, bool *err);

#endif //PW_VALIDATOR_VALIDATOR_MQ_H
