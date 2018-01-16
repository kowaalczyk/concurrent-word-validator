//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_VALIDATOR_MQ_H
#define PW_VALIDATOR_VALIDATOR_MQ_H

#include <mqueue.h>
#include <stdbool.h>
#include "config.h"

// [flag]-[pidstr]-[word]\0
#define VALIDATOR_MQ_BUFFSIZE (WORD_LEN_MAX + PID_STR_LEN + 4)

extern const char VALIDATOR_MQ_FLAG_HALT;
extern const char VALIDATOR_MQ_FLAG_START;
extern const char VALIDATOR_MQ_FLAG_FINISH_PASSED;
extern const char VALIDATOR_MQ_FLAG_FINISH_FAILED;


/**
 * Creates message queue for sending requests to validator,
 * if any error occurs during creation, closes all programs.
 * @param server - if true, queue will be read-only
 * @return descriptor of created queue
 */
extern mqd_t validator_mq_start(bool server);

/**
 * Sends a validation start request containing provided word to provided mq,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq - message queue created using validator_mq_start
 * @param word - word to validate
 */
extern void validator_mq_send_validation_start_request(mqd_t validator_mq, const char * word);

/**
 * Waits until a message is received via provided mq,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq - message queue created using validator_mq_start
 * @param buffer - buffer to which incoming message will be written
 * @param buffer_size - maximum length of message that can be written to buffer
 * @return size of received message
 */
extern ssize_t validator_mq_receive(mqd_t validator_mq, char * buffer, size_t buffer_size);

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
 * Closes provided queue and cleans its data,
 * if any error occurs during creation, closes all programs.
 * @param validator_mq
 */
extern void validator_mq_finish(mqd_t validator_mq);

#endif //PW_VALIDATOR_VALIDATOR_MQ_H
