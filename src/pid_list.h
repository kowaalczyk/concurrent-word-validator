//
// Created by kowal on 22.01.18.
//

#ifndef PW_VALIDATOR_PID_LIST_H
#define PW_VALIDATOR_PID_LIST_H

#include "config.h"

#define PID_LIST_EMPTY_ITEM_PID ((pid_t)(-1))

typedef struct pid_list_item{
    pid_t pid;
    struct pid_list_item *next;
} pid_list_item_t;

extern pid_list_item_t *pid_list_create(bool *err);

extern pid_t pid_list_emplace(pid_list_item_t *list, pid_t pid, bool *err);

extern void pid_list_destroy(pid_list_item_t *list);

#endif //PW_VALIDATOR_PID_LIST_H
