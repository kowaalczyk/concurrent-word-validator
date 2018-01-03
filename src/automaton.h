//
// Created by kowal on 29.12.17.
//

#ifndef PW_VALIDATOR_AUTO_H
#define PW_VALIDATOR_AUTO_H

#include <stdio.h>


// automaton struct parameters
#define STR_LEN_MAX 1000
#define ALPHABET_MAX_SIZE ('z'-'a')
#define STATES_MAX_SIZE 100
#define TRANSITIONS_MAX_SIZE (STATES_MAX_SIZE * ALPHABET_MAX_SIZE)
#define STR_STORAGE_VAL_OFFSET 1


// message queue parameters TODO: Move to separate header, TODO: for PID use http://www.cplusplus.com/reference/cstdlib/itoa/
/**
 * Request form: "[TYPE]-[PID]-[DATA]", where:
 * TYPE : {1, 2, 3} (1. is sent from tester, 2. from run, 3. sent from tester)
 * PID : (pid of a tester requesting the validation, in binary format)
 *
 */
#define VALIDATION_START_FLAG '1'
#define VALIDATION_FINISH_FLAG '2'
#define VALIDATION_HALT_FLAG '3'
#define VALIDATOR_INCOMING_REQUESTS_MQ_NAME "automata_validator_server_validation_request_q"
#define VALIDATOR_INCOMING_REQUESTS_MQ_BUFFSIZE (STR_LEN_MAX + sizeof(pid_t) + 1 + 3)


/**
 * Structure representing a single automaton.
 * States and word letters are indexed from 0 in order to minimize used memory.
 * States represented in C strings acceptable_states and transitions[i]
 * are offsetted by STR_STORAGE_VAL_OFFSET in order to
 * prevent misreading state 0 as '\0' C-string terminating char.
 */
typedef struct automaton {
    size_t alphabet_size;
    size_t states_size; /// size of all states in automaton
    size_t universal_states_size; /// 0..universal_states_size-1 := universal states, universal_states_size..states_size-1 := existential states
    char starting_state; /// starting state for validation
    char acceptable_states[STATES_MAX_SIZE]; /// '\0'-terminated, each char represents one state (offsetted)
    char transitions[TRANSITIONS_MAX_SIZE][STATES_MAX_SIZE]; /// transitions[state*(alphabet_size) + letter_normalized] := C-string containing all possible following states (offsetted)
} automaton;

#endif //PW_VALIDATOR_AUTO_H
