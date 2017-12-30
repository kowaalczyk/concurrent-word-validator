//
// Created by kowal on 27.12.17.
//

#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include "automaton.h"


/// checks if given state is universal in a given automata
bool is_universal(const automaton * a, const char state) {
    assert(state >= 0);
    assert(state < a->states_size);

    return state < a->universal_states_size;
}

/// check if given state is existential in a given automata
bool is_existential(const automaton * a, const char state) {
    assert(state >= 0);
    assert(state < a->states_size);

    return state >= a->universal_states_size;
}

/// iterates over acceptable states in automaton a, checking if the given state is acceptable, O(n)
bool is_acceptable(const automaton * a, const char state) {
    assert(state >= 0);
    assert(state < a->states_size);

    size_t acceptable_states_length = strlen(a->acceptable_states);
    int i;
    for(i=0; i<acceptable_states_length; i++) {
        if(a->acceptable_states[i] == state) {
            return true;
        }
    }
    assert(i == acceptable_states_length);
    return false;
}

/// returns string containing avaliable transitions (following states for current state and currently processed word) for given state in given automaton
const char * get_following_states(const automaton * a, const char state, const char word_letter) {
    assert(state >= 0);
    assert(state < a->states_size);
    assert(word_letter >= 'a');
    assert(word_letter <= 'z');

    // TODO: Might need to normalize the alphabet to start from 0
    return a->transitions[state*a->alphabet_size + word_letter];
}

/// recursive helper for word validation, use 'accept' function
bool accept_rec(const automaton *a, const char *word, const char *state) {
    size_t w_len = strlen(word);
    size_t depth = strlen(state)-1;

    if(depth > w_len) {
        return is_acceptable(a, state[depth]);
    }
    const char * following_states = get_following_states(a, state[depth], word[depth]);
    size_t following_states_length = strlen(following_states);
    if(is_existential(a, state[depth])) {
        // need to accept_rec any of following states
        int i;
        for(i=0; i<following_states_length; i++) {
            // TODO: Unit test this (!!!)
            char following_state[STR_LEN_MAX];
            strcpy(following_state, state);
            strcat(following_state, &following_states[i]);
            assert(strlen(following_state) == strlen(state)+1);

            if(accept_rec(a, word, following_state)) {
                return true;
            }
        }
        assert(i == following_states_length);
        return false; // no state was accepted
    }
    assert(is_universal(a, state[depth]));
    int i;
    for(i=0; i<following_states_length; i++) {
        // TODO: Unit test this (!!!)
        char following_state[STR_LEN_MAX];
        strcpy(following_state, state);
        strcat(following_state, &following_states[i]);
        assert(strlen(following_state) == strlen(state)+1);

        if(!accept_rec(a, word, following_state)) {
            return false;
        }
    }
    assert(i == following_states_length);
    return true; // all states were accepted
}

/// checks if given automaton accepts given word
bool accept(const automaton * a, const char * word) {
    return accept_rec(a, word, &(a->starting_state));
}


int main() {
    // TODO: Get answer queue name from program args
    // TODO: Create an answer queue so that validator is not blocked


    // TODO: Implement automata word validation
    // TODO: Consider validating words in parallel (don't make a fork bomb though)

    // TODO: Send answer, close queues and return
    return 0;
}
