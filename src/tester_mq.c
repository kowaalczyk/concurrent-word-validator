//
// Created by kowal on 13.01.18.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include "tester_mq.h"


#define TESTER_MQ_PERMISSIONS 0666

static const char TESTER_MQ_NAME_PREFIX[] = "/pw_validator_tester_mq_"; // make sure TESTER_MQ_NAME_PREFIX_LEN is set correctly


void pidstr(pid_t pid, char *target) {
    sprintf(target, "%0*d", (int)PID_STR_LEN, pid);
}

void tester_mq_get_name_from_pid(pid_t pid, char *target) {
    assert(target != NULL);

    memcpy(target, TESTER_MQ_NAME_PREFIX, TESTER_MQ_NAME_PREFIX_LEN);
    pidstr(pid, target + TESTER_MQ_NAME_PREFIX_LEN);
}

mqd_t tester_mq_start(bool server, const char *tester_mq_name, bool *err) {
    assert(tester_mq_name != NULL && err != NULL);

    int tmp_err = 0;
    mqd_t queue;

    if(server) {
        queue = mq_open(tester_mq_name, O_RDONLY | O_CREAT, TESTER_MQ_PERMISSIONS, NULL);
        INT_FAIL_IF(queue == -1);

        // resize queue message size if it is too small
        struct mq_attr tester_mq_attrs;
        tmp_err = mq_getattr(queue, &tester_mq_attrs);
        INT_FAIL_IF(tmp_err == -1);
        INT_FAIL_IF(tester_mq_attrs.mq_msgsize < sizeof(tester_mq_msg));
    } else {
        queue = mq_open(tester_mq_name, O_WRONLY);
        INT_FAIL_IF(queue == -1);
    }
    assert(queue != -1);
    return queue;
}

size_t tester_mq_get_buffsize(mqd_t queue, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;
    struct mq_attr tmp;

    tmp_err = mq_getattr(queue, &tmp);
    INT_FAIL_IF(tmp_err == -1);
    return (size_t) tmp.mq_msgsize;
}

void tester_mq_finish(bool unlink, mqd_t tester_mq, const char *tester_mq_name, bool *err) {
    assert(err != NULL);

    int tmp_err = 0;

    tmp_err = mq_close(tester_mq);
    if(unlink) {
        tmp_err = mq_unlink(tester_mq_name);
    }
    VOID_FAIL_IF(tmp_err == -1);
}

void tester_mq_send(mqd_t tester_mq, const char *word, bool completed, size_t total_processed, bool ignored, bool accepted,
                    bool *err) {
    assert(err != NULL);

    int tmp_err = 0;
    tester_mq_msg msg;
    memset(&msg, 0, sizeof(tester_mq_msg)); // to prevent valgrind errors
    msg = (tester_mq_msg){completed, total_processed, ignored, accepted, ""};
    if(word != NULL) {
        memcpy(msg.word, word, strlen(word));
    }

    tmp_err = mq_send(tester_mq, (const char *)&msg, sizeof(tester_mq_msg), NORMAL_MQ_PRIORITY);
    VOID_FAIL_IF(tmp_err == -1);
}

ssize_t tester_mq_receive(mqd_t tester_mq, tester_mq_msg *msg, bool *err) {
    assert(msg != NULL && err != NULL);

    size_t buffsize = tester_mq_get_buffsize(tester_mq, err);
    INT_FAIL_IF(*err);

    char buff[buffsize];
    ssize_t request_ret = mq_receive(tester_mq, buff, buffsize, NULL);
    INT_FAIL_IF(request_ret <= 0);

    memcpy(msg, buff, sizeof(tester_mq_msg));
    return request_ret;
}
