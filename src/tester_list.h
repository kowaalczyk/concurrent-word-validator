//
// Created by kowal on 19.01.18.
//

#ifndef PW_VALIDATOR_TESTER_LIST_H
#define PW_VALIDATOR_TESTER_LIST_H

#include <zconf.h>
#include <stdbool.h>

typedef struct tester{
    pid_t pid;
    size_t rcd;
    size_t acc;
    int word_bal;
    bool completed;
} tester_t;

typedef struct tester_list{
    tester_t *this;
    struct tester_list *next;
} tester_list_t;

/**
 * Creates an empty tester list.
 * @param err
 * @return - pointer to created list
 */
extern tester_list_t * tester_list_create(bool *err);

/**
 * Creates an element within a list with provided parameters.
 * @param list
 * @param pid
 * @param rcd
 * @param acc
 * @param err
 */
extern void
tester_list_emplace(tester_list_t *list, pid_t pid, size_t rcd, size_t acc, int word_bal, bool completed, bool *err);

/**
 * Checks if element with provided pid is present in a list.
 * @param list
 * @param pid
 * @return - pointer to found element | NULL if matching element exists
 */
extern tester_t * tester_list_find(tester_list_t *list, pid_t pid);

/**
 * Prints log of tester list contents.
 * @param list
 */
extern void tester_list_print_log(const tester_list_t *list);

/**
 * Removes provided list from memory.
 * @param list
 */
extern void tester_list_destroy(tester_list_t *list);

#endif //PW_VALIDATOR_TESTER_LIST_H
