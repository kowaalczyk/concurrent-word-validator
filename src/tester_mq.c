//
// Created by kowal on 13.01.18.
//

#include <string.h>
#include <assert.h>
#include "tester_mq.h"
#include "config.h"

void tester_mq_get_name_from_pid(pid_t pid, char *target) {
    assert(sizeof(target) == TESTER_MQ_NAME_LEN);

    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    pidstr(pid, target + TESTER_MQ_NAME_PREFIX_LEN);
}

void tester_mq_get_name_from_message(const char *pid_msg_part, char *target) {
    assert(sizeof(target) == TESTER_MQ_NAME_LEN);

    // TODO: Extract part of message containing PID and copy it to target name
}

mqd_t tester_mq_start(bool server, const char *tester_mq_name) {
    // TODO
    //    char * response_mq_name = create_response_queue_name(getpid());
//    mqd_t tester_mq = mq_open(response_mq_name, O_RDONLY | O_CREAT);
//    if(tester_mq == -1) {
//        syserr("TESTER: Failed to create response queue");
//    }
    return 0;
}

void tester_mq_finish(mqd_t tester_mq) {
    // TODO
//    if (mq_close(response_mq)) syserr("TESTER: Failed to close queue");
//    if (mq_unlink(response_mq_name)) syserr("TESTER: Failed to unlink queue");
}

void tester_mq_send_halt(mqd_t tester_mq) {
    // TODO
}

void tester_mq_send_validation_result(mqd_t tester_mq, const char *buffer, size_t buffer_size) {
    // TODO
}

bool tester_mq_received_halt(const char *buffer, ssize_t buffer_size) {
    // TODO
    return 0;
}

bool tester_mq_received_validation_result(const char *buffer, ssize_t buffer_size) {
    // TODO
    return 0;
}

