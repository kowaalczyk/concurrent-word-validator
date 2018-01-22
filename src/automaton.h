//
// Created by kowal on 29.12.17.
//

#ifndef PW_VALIDATOR_AUTO_H
#define PW_VALIDATOR_AUTO_H

#include <stdio.h>
#include <stdbool.h>

#define ALPHABET_SIZE ('z'-'a'+1)
#define STATES_SIZE 101
#define TRANSITIONS_SIZE (STATES_SIZE * ALPHABET_SIZE)
#define STR_STORAGE_VAL_OFFSET 1

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
    char acceptable_states[STATES_SIZE]; /// '\0'-terminated, each char represents one state (offsetted)
    char transitions[TRANSITIONS_SIZE][STATES_SIZE]; /// transitions[state*(alphabet_size) + letter_normalized] := C-string containing all possible following states (offsetted)
} automaton;

/**
 * Loads automaton from stdin to provided pointer, no dynamic memory allocation.
 * @param a 
 * @param err 
 */
extern void load_automaton(automaton *ptr, bool *err);

#endif //PW_VALIDATOR_AUTO_H
