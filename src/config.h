//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_CONFIG_H
#define PW_VALIDATOR_CONFIG_H

#include <mqueue.h> // TODO: Make sure there is no better lib for pid_t
#include <memory.h>
#include <stdbool.h>

// TODO: All common #defines go here
// TODO: Refactor #defines to include project prefix

#define WORD_LEN_MAX 1000

// 1 byte holds 3 digits at maximum, so 3x sizeof in bytes is enough to hold pid_t as string
#define PID_STR_LEN (sizeof(pid_t)*3)

#define VALIDATION_PASSED_FLAG 'A'
#define VALIDATION_FAILED_FLAG 'N'
#define HALT_FLAG '!'

#define HALT_FLAG_PRIORITY 1
#define NORMAL_FLAG_PRIORITY 1

/**
 * Macros for error handling used within the project
 */
#define INT_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return -1;}} while(false)
#define VOID_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return;}} while(false)
#define PTR_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; return NULL;}} while(false)
#define HANDLE_ERR(err_handler) do {if(err) {(err_handler)();}} while(false)
#define HANDLE_ERR_EXIT_WITH_MSG(message) do {if(err) {fprintf(stderr, "%s - error number: %d (%s)\n", message, errno, strerror(errno)); exit(-1);}} while(false)
#define HANDLE_ERR_DECREMENT_CONTINUE(to_decrement) do {if(err) (to_decrement)--; continue;} while(false)

/**
 * Converts provided pid to formatted C-string and writes it to the target
 * @param pid
 * @param target - pointer to string that will store the data
 */
void pidstr(pid_t pid, char * target);

#endif //PW_VALIDATOR_CONFIG_H
