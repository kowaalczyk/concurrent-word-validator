//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_CONFIG_H
#define PW_VALIDATOR_CONFIG_H

#include <mqueue.h> // TODO: Make sure there is no better lib for pid_t

// TODO: All common #defines go here
// TODO: Refactor #defines to include project prefix

#define WORD_LEN_MAX 1000
#define PID_STR_LEN (sizeof(pid_t))

#define VALIDATION_PASSED_FLAG 'A'
#define VALIDATION_FAILED_FLAG 'N'
#define HALT_FLAG '!'

#define HALT_FLAG_PRIORITY 7

void pidstr(pid_t pid, char * target);

#endif //PW_VALIDATOR_CONFIG_H
