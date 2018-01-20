//
// Created by kowal on 19.01.18.
//

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tester_list.h"
#include "config.h"

#define TESTER_LIST_ALLOC ((tester_list_t*)malloc(sizeof(tester_list_t)))
#define TESTER_LIST_ASSERT_OK assert(list != NULL && list->this == NULL)

tester_list_t *tester_list_create(bool *err) {
    tester_list_t *guard = TESTER_LIST_ALLOC;
    PTR_FAIL_IF(guard == NULL);

    guard->this = NULL;
    guard->next = NULL;
    return guard;
}

void tester_list_emplace(tester_list_t *list, pid_t pid, size_t rcd, size_t acc, int word_bal, bool *err) {
    TESTER_LIST_ASSERT_OK;

    tester_list_t *new_first = TESTER_LIST_ALLOC;
    VOID_FAIL_IF(new_first == NULL);

    tester_t *new_tester = (tester_t*)malloc(sizeof(tester_t));
    VOID_FAIL_IF(new_tester == NULL);
    new_tester->pid = pid;
    new_tester->rcd = rcd;
    new_tester->acc = acc;
    new_tester->word_bal = word_bal;

    tester_list_t * old_first = list->next;
    new_first->this = new_tester;
    new_first->next = old_first;
    list->next = new_first;
}

tester_t *tester_list_find(tester_list_t *list, pid_t pid) {
    TESTER_LIST_ASSERT_OK;

    tester_list_t *iter = list->next;
    while(iter != NULL) {
        if(iter->this != NULL && iter->this->pid == pid) {
            return iter->this;
        }
        iter = iter->next;
    }
    return NULL;
}

void tester_list_print_log(const tester_list_t *list) {
    TESTER_LIST_ASSERT_OK;

    tester_list_t *iter = list->next;
    while(iter != NULL) {
        if(iter->this != NULL) {
            printf("PID: %d\nRcd: %zu\n Acc: %zu\n", iter->this->pid, iter->this->rcd, iter->this->acc);
        }
        iter = iter->next;
    }
}

void tester_list_destroy_rec(tester_list_t *list) {
    if(list->next != NULL) {
        tester_list_destroy_rec(list->next);
    }
    free(list->this);
    free(list);
}

void tester_list_destroy(tester_list_t *list) {
    TESTER_LIST_ASSERT_OK;

    tester_list_destroy_rec(list);
}

