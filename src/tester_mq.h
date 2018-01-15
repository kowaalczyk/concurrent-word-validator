//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_TESTER_MQ_H
#define PW_VALIDATOR_TESTER_MQ_H

#include <mqueue.h>
#include <stdbool.h>

#define TESTER_MQ_NAME_PREFIX_LEN 24
#define TESTER_MQ_NAME_LEN (TESTER_MQ_NAME_PREFIX_LEN + PID_STR_LEN)

// [word] [flag]\n\0
#define TESTER_MQ_BUFFSIZE (WORD_LEN_MAX + 3)

const char TESTER_MQ_FLAG_HALT = '!';
const char TESTER_MQ_FLAG_PASSED = 'A';
const char TESTER_MQ_FLAG_FAILED = 'N';

const char TESTER_MQ_NAME_PREFIX[] = "/pw_validator_tester_mq_"; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly

extern void tester_mq_get_name_from_pid(pid_t pid, char * target);

extern void tester_mq_get_name_from_message(const char * pid_msg_part, char * target);

extern mqd_t tester_mq_start(bool server, const char * tester_mq_name);

extern void tester_mq_send_halt(mqd_t tester_mq);

extern void tester_mq_send_validation_result(mqd_t tester_mq, const char * buffer, size_t buffer_size);

extern ssize_t tester_mq_receive(mqd_t tester_mq, char * buffer, size_t buffer_size);

extern bool tester_mq_received_halt(const char * buffer, ssize_t buffer_size);

extern bool tester_mq_received_validation_result(const char * buffer, ssize_t buffer_size);

extern void tester_mq_finish(mqd_t tester_mq);

#endif //PW_VALIDATOR_TESTER_MQ_H
