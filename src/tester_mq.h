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


typedef struct tester_mq_msg{
    bool completed;
    bool ignored;
    bool accepted;
    char word[WORD_LEN_MAX];
} tester_mq_msg;

/**
 * Creates C-string containing tester_mq name created based on provided pid
 * @param pid
 * @param target
 */
extern void tester_mq_get_name_from_pid(pid_t pid, char * target);

/**
 * Creates C-string containing tester_mq name, created based on provided pid string
 * @param pid_str
 * @param target
 */
extern void tester_mq_get_name_from_pidstr(const char *pid_str, char *target);

/**
 * Creates tester mq, for server read-only with initial setup, for client write-only.
 * @param server - if true, creates version for the server
 * @param tester_mq_name
 * @return - descriptor of created tester mq
 */
extern mqd_t tester_mq_start(bool server, const char *tester_mq_name, bool *err);

extern void tester_mq_send_completed(mqd_t tester_mq, bool *err); // TODO: Remove

/**
 * Blocking.
 * Sends message with provided data to specified tester_mq
 * @param tester_mq
 * @param word
 * @param completed
 * @param ignored
 * @param accepted
 * @param err
 */
extern void tester_mq_send(mqd_t tester_mq, const char *word, bool completed, bool ignored, bool accepted, bool *err);

/**
 * Blocking.
 * Reveives message from specified tester_mq
 * @param tester_mq
 * @param msg
 * @param err
 * @return
 */
extern ssize_t tester_mq_receive(mqd_t tester_mq, tester_mq_msg *msg, bool *err);

// TODO: Remove both:
extern bool tester_mq_received_halt(const char * buffer, ssize_t buffer_size);
extern bool tester_mq_received_validation_result(const char * buffer, ssize_t buffer_size);

/**
 * Closes provided tester mq. In server version, frees tester mq name.
 * Make sure server version is executed after all clients are closed.
 * @param server - if true, performs server version of the function
 * @param tester_mq
 * @param tester_mq_name
 */
void tester_mq_finish(bool server, mqd_t tester_mq, const char *tester_mq_name, bool *err);

#endif //PW_VALIDATOR_TESTER_MQ_H
