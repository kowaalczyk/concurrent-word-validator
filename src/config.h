//
// Created by kowal on 13.01.18.
//

#ifndef PW_VALIDATOR_CONFIG_H
#define PW_VALIDATOR_CONFIG_H

// TODO: All common #defines go here
// TODO: Refactor #defines to include project prefix

#define WORD_LEN_MAX 1000
#define PID_STR_LEN_MAX (sizeof(pid_t))

const char SYMBOL_VALIDATION_PASSED = 'A';
const char SYMBOL_VALIDATION_FAILED = 'N';

// TODO: Delete/reformat:
/**
 * Queue naming convention: "/PW_VALIDATOR_[REQUEST|RESPONSE]_[pid of opener]
 */

#endif //PW_VALIDATOR_CONFIG_H
