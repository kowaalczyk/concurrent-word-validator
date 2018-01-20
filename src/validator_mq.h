//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_VALIDATOR_MQ_H
#define PW_VALIDATOR_VALIDATOR_MQ_H

#include <mqueue.h>
#include <stdbool.h>
#include "config.h"


/**
 * Represents a single message sent via validator_mq
 */
typedef struct validator_mq_msg{
    bool start;
    bool halt;
    bool finished;
    bool accepted;
    pid_t tester_pid;
    char word[WORD_LEN_MAX];
} validator_mq_msg;


/**
 * Creates validator_mq, server version is read-only and needs to be performed before any client queue creation
 * @param server - if true, creates version for the server
 * @return descriptor of created queue
 */
extern mqd_t validator_mq_start(bool server, bool *err);

/**
 * Blocking.
 * Sends message with provided data to specified validator_mq
 * @param validator_mq
 * @param start
 * @param halt
 * @param finished
 * @param accepted
 * @param tester_pid
 * @param word
 * @param err
 */
extern void validator_mq_send(mqd_t validator_mq, bool start, bool halt, bool finished, bool accepted, pid_t tester_pid,
                              const char *word, bool *err);

/**
 * Blocking.
 * Receives message from specified validator_mq
 * @param validator_mq
 * @param msg
 * @param err
 * @return
 */
extern ssize_t validator_mq_receive(mqd_t validator_mq, validator_mq_msg *msg, bool *err);

/**
 * Closes provided validator mq, server version needs to be executed after all other clients are finished
 * @param validator_mq
 * @param unlink - if true, unlinks queue name
 * @param err
 */
extern void validator_mq_finish(bool unlink, mqd_t validator_mq, bool *err);

#endif //PW_VALIDATOR_VALIDATOR_MQ_H
