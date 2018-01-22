//
// Created by kowal on 22.01.18.
//

#include <assert.h>
#include <stdlib.h>
#include "pid_list.h"

#define PIDLIST_ALLOC ((pid_list_item_t*)malloc(sizeof(pid_list_item_t)))


pid_list_item_t *pid_list_create(bool *err) {
    pid_list_item_t *guard = PIDLIST_ALLOC;
    PTR_FAIL_IF(guard == NULL);

    guard->pid = PID_LIST_EMPTY_ITEM_PID;
    guard->next = NULL;
    return guard;
}

pid_t pid_list_emplace(pid_list_item_t *list, pid_t pid, bool *err) {
    assert(list != NULL && list->pid == PID_LIST_EMPTY_ITEM_PID);

    pid_list_item_t *new_first = PIDLIST_ALLOC;
    PTR_FAIL_IF(new_first == NULL);

    pid_list_item_t *old_first = list->next;
    new_first->pid = pid;
    new_first->next = old_first;
    list->next = new_first;

    return new_first->pid;
}

void pid_list_destroy_rec(pid_list_item_t *list_item) {
    if(list_item->next != NULL) {
        pid_list_destroy_rec(list_item->next);
    }
    free(list_item);
}

void pid_list_destroy(pid_list_item_t *list) {
    assert(list != NULL && list->pid == PID_LIST_EMPTY_ITEM_PID);

    pid_list_destroy_rec(list);
}
