//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_CONFIG_H
#define PW_VALIDATOR_CONFIG_H

#include <mqueue.h>
#include <memory.h>
#include <stdbool.h>

// TODO: Refactor #defines to include project prefix

#define WORD_LEN_MAX 1000

#define NORMAL_MQ_PRIORITY 1

/**
 * Macros for error handling used within the project
 */
#define INT_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return -1;}} while(false)
#define VOID_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return;}} while(false)
#define PTR_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return NULL;}} while(false)
#define HANDLE_ERR(err_handler) do {if(err) {(err_handler)();}} while(false)
#define HANDLE_ERR_EXIT_WITH_MSG(message) do {if(err) {fprintf(stderr, "%s - error number: %d (%s)\n", message, errno, strerror(errno)); exit(-1);}} while(false)
#define HANDLE_ERR_EXIT_ERRNO_WITH_MSG(message) do {if(err) {fprintf(stderr, "%s - error number: %d (%s)\n", message, errno, strerror(errno)); exit(-errno);}} while(false)
#define HANDLE_ERR_DECREMENT_BREAK(to_decrement) do {if(err) (to_decrement)--; break;} while(false)
#define HANDLE_ERR_DECREMENT_CONTINUE(to_decrement) do {if(err) (to_decrement)--; continue;} while(false)

typedef struct comm_summary{
    size_t snt;
    size_t rcd;
    size_t acc;
} comm_sumary_t;

inline void print_comm_summary(const comm_sumary_t *comm_summary) {
    printf("Snt: %zu\nRcd: %zu\nAcc: %zu\n", comm_summary->snt, comm_summary->rcd, comm_summary->acc);
}

#endif //PW_VALIDATOR_CONFIG_H
