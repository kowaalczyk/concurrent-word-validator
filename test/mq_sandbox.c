//
// Created by kowal on 17.01.18.
//

#include <zconf.h>
#include <stdlib.h>
#include <wait.h>
#include <mqueue.h>
#include <stdio.h>
#include <assert.h>
#include "../src/err.h"

static const char * NAME = "/pw_sandbox";

void async_runner() {
    int tmp;

    mqd_t queue = mq_open(NAME, O_WRONLY);
    assert(queue != -1);

    tmp = mq_send(queue, "dupa\n\0", 6, 1);
    assert(tmp != -1);

    tmp = mq_close(queue);
    assert(tmp != -1);
}

void async_start() {
    switch(fork()) {
        case -1:
            syserr("Error in fork");
        case 0:
            async_runner();
            exit(0);
        default:
            break;
    }
}

int main() {
    int tmp;
    ssize_t tmp2;

    async_start();

    mqd_t queue;
    queue = mq_open(NAME, O_RDONLY | O_CREAT, 0666, NULL);
    assert(queue != -1);

    struct mq_attr queue_attrs;
    tmp = mq_getattr(queue, &queue_attrs);
    assert(tmp != -1);

    printf("%li %li %li %li\n",
           queue_attrs.mq_msgsize,
           queue_attrs.mq_curmsgs,
           queue_attrs.mq_maxmsg,
           queue_attrs.mq_flags
    );

    char ans_buff[5];
    tmp2 = mq_receive(queue, ans_buff, 8193, NULL); // TODO: size buora odbioru większy niż domuślny wiadomości w kolejce
    assert(tmp2 != -1);

    printf("%s\n", ans_buff);

    tmp = mq_close(queue);
    assert(tmp != -1);
    tmp = mq_unlink(NAME);
    assert(tmp != -1);
    wait(NULL);
    return 0;
}