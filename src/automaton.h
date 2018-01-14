//
// Created by kowal on 29.12.17.
//

#ifndef PW_VALIDATOR_AUTO_H
#define PW_VALIDATOR_AUTO_H

#include <stdio.h>

#define ALPHABET_SIZE ('z'-'a')
#define STATES_SIZE 100
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
 * Creates automaton based on data from stdin.
 * Automaton cannot be modified later, and can be freed only via delete_automaton.
 * @return pointer to created automaton
 */
extern const automaton * load_automaton(); // TODO: Check error handling

/**
 * Frees memory allocated for automaton, disregards const specifier.
 * @param to_delete
 */
extern void delete_automaton(const automaton * to_delete); // TODO: Make sure this will compile

#endif //PW_VALIDATOR_AUTO_H
