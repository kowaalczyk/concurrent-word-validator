//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_TESTER_MQ_H
#define PW_VALIDATOR_TESTER_MQ_H

#include <mqueue.h>
#include <stdbool.h>
#include "config.h"


#define TESTER_MQ_NAME_PREFIX_LEN 24
#define TESTER_MQ_NAME_LEN (TESTER_MQ_NAME_PREFIX_LEN + PID_STR_LEN)


/**
 * Represents a single message sent via tester_mq
 */
typedef struct tester_mq_msg{
    bool completed; /// message flag menanig there will be no more requests from validator to this mq
    size_t total_processed; /// if completed flag is set to true, contains total number of words tester should have received
    bool ignored; /// message flag meaning that word in a message was ignored by validator
    bool accepted; /// message flag meaning word was accepted by automaton in a validator
    char word[WORD_LEN_MAX+1]; /// word ended with '\0'
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
 * @param total_processed
 * @param ignored
 * @param accepted
 * @param err
 */
extern void tester_mq_send(mqd_t tester_mq, const char *word, bool completed, size_t total_processed, bool ignored, bool accepted,
                           bool *err);

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
