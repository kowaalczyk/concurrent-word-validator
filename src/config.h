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


#ifdef PW_ENABLE_LOGGING
static const bool enable_logging = true;
#else
static const bool enable_logging = false;
#endif /* PW_ENABLE_LOGGING */

/// standard logging function used for debug
extern void log_formatted(const char *fmt, ...);


/**
 * Macros for error handling used within the project
 */
#define INT_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; log_formatted("Fail in %s, function: %s", __FILE__, __func__); return -1;}} while(false)
#define VOID_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; log_formatted("Fail in %s, function: %s", __FILE__, __func__); return;}} while(false)
#define PTR_FAIL_IF(x) do {if(x) {if(err != NULL) *err = true; log_formatted("Fail in %s, function: %s", __FILE__, __func__); return NULL;}} while(false)
#define HANDLE_ERR(err_handler) do {if(err) {(err_handler)();}} while(false)
#define HANDLE_ERR_EXIT_WITH_MSG(message) do {if(err) {fprintf(stderr, "%s - error number: %d (%s)\n", message, errno, strerror(errno)); exit(EXIT_FAILURE);}} while(false)
#define HANDLE_ERR_DECREMENT_BREAK(to_decrement) do {if(err) (to_decrement)--; break;} while(false)
#define HANDLE_ERR_DECREMENT_CONTINUE(to_decrement) do {if(err) (to_decrement)--; continue;} while(false)

/// struct containing standard communication summary for application within a project
typedef struct comm_summary{
    size_t snt;
    size_t rcd;
    size_t acc;
} comm_sumary_t;

/// prints communication summary to stdout
extern void print_comm_summary(const comm_sumary_t *comm_summary);


#endif //PW_VALIDATOR_CONFIG_H
