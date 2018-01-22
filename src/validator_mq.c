//
// Created by kowal on 13.01.18.
//

#include "validator_mq.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#define VALIDATOR_MQ_PERMISSIONS 0666

static const char VALIDATOR_MQ_NAME[] = "/pw_validator_validator_mq_single";


mqd_t validator_mq_start(bool server, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;
    mqd_t queue;

    if(server) {
        queue = mq_open(VALIDATOR_MQ_NAME, O_RDONLY | O_CREAT, VALIDATOR_MQ_PERMISSIONS, NULL);
        INT_FAIL_IF(queue == -1);

        // resize queue message size if it is too small
        struct mq_attr validator_mq_attrs;
        tmp_err = mq_getattr(queue, &validator_mq_attrs);
        INT_FAIL_IF(tmp_err == -1);
        INT_FAIL_IF(validator_mq_attrs.mq_msgsize < sizeof(validator_mq_msg));
    } else {
        queue = mq_open(VALIDATOR_MQ_NAME, O_WRONLY);
        INT_FAIL_IF(queue == -1);
    }
    assert(queue != -1);
    return queue;
}

size_t validator_mq_get_buffsize(mqd_t queue, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;
    struct mq_attr tmp;

    tmp_err = mq_getattr(queue, &tmp);
    INT_FAIL_IF(tmp_err == -1);
    return (size_t) tmp.mq_msgsize;
}

void validator_mq_send(mqd_t validator_mq, bool start, bool halt, bool completed, bool finish, bool accepted,
                       pid_t tester_pid, const char *word, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;
    validator_mq_msg msg;
    memset(&msg, 0, sizeof(validator_mq_msg)); // to prevent valgrind errors
    msg = {start, halt, completed, finish, accepted, tester_pid, ""};
    if(word != NULL) {
        memcpy(msg.word, word, strlen(word));
    }

    tmp_err = mq_send(validator_mq, (const char *)&msg, sizeof(validator_mq_msg), NORMAL_MQ_PRIORITY);
    VOID_FAIL_IF(tmp_err == -1);
}

// TODO: Optimize memory usage: call this function from the other to prevent duplicating msg struct
void validator_mq_send_msg(mqd_t validator_mq, const validator_mq_msg *msg, bool *err) {
    validator_mq_send(validator_mq, msg->start, msg->halt, 0, msg->finish, msg->accepted, msg->tester_pid, msg->word,
                      err);
}

ssize_t validator_mq_receive(mqd_t validator_mq, validator_mq_msg *msg, bool *err) {
    assert(msg != NULL && err != NULL);

    size_t buffsize = validator_mq_get_buffsize(validator_mq, err);
    INT_FAIL_IF(*err);

    char buff[buffsize];
    ssize_t request_ret = mq_receive(validator_mq, buff, buffsize, NULL);
    INT_FAIL_IF(request_ret <= 0);

    memcpy(msg, buff, sizeof(validator_mq_msg));
    return request_ret;
}

void validator_mq_finish(bool unlink, mqd_t validator_mq, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;

    tmp_err = mq_close(validator_mq);
    if(unlink) {
        // intentionally performed before error checking to efficiently free resources in case of error
        tmp_err = mq_unlink(VALIDATOR_MQ_NAME);
    }
    VOID_FAIL_IF(tmp_err == -1);
}
