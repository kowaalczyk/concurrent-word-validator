//
// Created by kowal on 29.12.17.
//

#ifndef PW_VALIDATOR_AUTO_H
#define PW_VALIDATOR_AUTO_H

#include <stdio.h>

#define STR_LEN_MAX 1000
#define ALPHABET_MAX_SIZE ('z'-'a')
#define STATES_MAX_SIZE 100
#define TRANSITIONS_MAX_SIZE (STATES_MAX_SIZE * ALPHABET_MAX_SIZE)

typedef struct automaton {
    size_t alphabet_size;
    size_t states_size; /// size of all states in automaton
    size_t universal_states_size; /// 0..universal_states_size-1 := universal states, universal_states_size..states_size-1 := existential states
    char starting_state; /// starting state for validation
    char acceptable_states[STATES_MAX_SIZE]; /// '\0'-terminated, each char represents one state, states are indexed from 0
    char transitions[TRANSITIONS_MAX_SIZE][STATES_MAX_SIZE]; /// transitions[state*(alphabet_size) + letter_normalized] := '\0'-terminated string containing all possible following states (as chars)
} automaton;

#endif //PW_VALIDATOR_AUTO_H
