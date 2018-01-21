//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_TESTER_MQ_H
#define PW_VALIDATOR_TESTER_MQ_H

#include <mqueue.h>
#include <stdbool.h>
#include "config.h"


// 1 byte holds 3 decimal digits at maximum, so 3x sizeof in bytes is enough to hold pid_t as string
#define PID_STR_LEN (sizeof(pid_t)*3)
#define TESTER_MQ_NAME_PREFIX_LEN 24
#define TESTER_MQ_NAME_LEN (TESTER_MQ_NAME_PREFIX_LEN + PID_STR_LEN)


/**
 * Represents a single message sent via tester_mq
 */
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
 * Creates tester mq, server version is read-only and needs to be performed before any client queue creation
 * @param server - if true, creates version for the server
 * @param tester_mq_name
 * @return descriptor of created tester mq
 */
extern mqd_t tester_mq_start(bool server, const char *tester_mq_name, bool *err);

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
 * Receives message from specified tester_mq
 * @param tester_mq
 * @param msg
 * @param err
 * @return
 */
extern ssize_t tester_mq_receive(mqd_t tester_mq, tester_mq_msg *msg, bool *err);

/**
 * Closes provided tester mq, server version needs to be executed after all other clients are finished
 * @param unlink - if true, unlinks queue name
 * @param tester_mq
 * @param tester_mq_name
 */
extern void tester_mq_finish(bool unlink, mqd_t tester_mq, const char *tester_mq_name, bool *err);

#endif //PW_VALIDATOR_TESTER_MQ_H