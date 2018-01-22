//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include "config.h"
#include "validator_mq.h"
#include "automaton.h"


/// checks if given state is universal in a given automata
static bool is_universal(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
    return state < a->universal_states_size;
}

/// check if given state is existential in a given automata
static bool is_existential(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
    return state >= a->universal_states_size;
}

/// iterates over acceptable states in automaton a, checking if the given state is acceptable, O(n)
static bool is_acceptable(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

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
static const char * get_following_states(const automaton * a, char state, char word_letter) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);
    assert(word_letter >= 'a');
    assert(word_letter <= 'z');

    state -= STR_STORAGE_VAL_OFFSET;
    word_letter -= 'a';
    return a->transitions[state*a->alphabet_size + word_letter];
}

/// recursive helper for word validation, use 'accept' function
static bool accept_rec(const automaton *a, const char *word, const char *state_list) {
    size_t w_len = strlen(word);
    size_t depth = strlen(state_list)-1;

    if(depth >= w_len) {
        return is_acceptable(a, state_list[depth]);
    }
    const char * following_states = get_following_states(a, state_list[depth], word[depth]);
    size_t following_states_length = strlen(following_states);
    if(is_existential(a, state_list[depth])) {
        // need to accept_rec any of following states
        int i;
        for(i=0; i<following_states_length; i++) {
            char state_list_extended[WORD_LEN_MAX+2];
            strcpy(state_list_extended, state_list);
            size_t fs_len = strlen(state_list_extended);
            // append one of possible following states to current state_list
            state_list_extended[fs_len] = following_states[i];
            state_list_extended[fs_len+1] = '\0';
            assert(strlen(state_list_extended) == strlen(state_list)+1);

            // TODO: fork()
            if(accept_rec(a, word, state_list_extended)) {
                return true;
            }
        }
        assert(i == following_states_length);
        return false; // no state_list was accepted
    }
    assert(is_universal(a, state_list[depth]));
    int i;
    for(i=0; i<following_states_length; i++) {
        char state_list_extended[WORD_LEN_MAX+2];
        strcpy(state_list_extended, state_list);
        size_t fs_len = strlen(state_list_extended);
        // append one of possible following states to current state_list
        state_list_extended[fs_len] = following_states[i];
        state_list_extended[fs_len+1] = '\0';
        assert(strlen(state_list_extended) == strlen(state_list)+1);

        // TODO: fork()
        if(!accept_rec(a, word, state_list_extended)) {
            return false;
        }
    }
    assert(i == following_states_length);
    return true; // all states were accepted
}

/// checks if given automaton accepts given word
static bool accept(const automaton * a, const char * word) {
    char state_list[WORD_LEN_MAX+2]; // states list for given word is equal its length
    state_list[0] = a->starting_state;
    state_list[1] = '\0';
    return accept_rec(a, word, state_list);
}


int main() {
    // TODO: Use signals in error handling (if unable to send to validator, everything should die)

    bool err = false;
    ssize_t tmp_err;
    mqd_t validator_mq;
    automaton a;
    validator_mq_msg prepared_msg;

    validator_mq = validator_mq_start(false, &err);
    HANDLE_ERR_EXIT_WITH_MSG("RUN: Failed to open validator mq");

    // receive automaton via pipe
    tmp_err = read(0, &a, sizeof(automaton));
    if(tmp_err != sizeof(automaton)) {
        HANDLE_ERR_EXIT_WITH_MSG("RUN: Invalid read from pipe - automaton");
    }

    // receive prepared message via pipe
    tmp_err = read(0, &prepared_msg, sizeof(validator_mq_msg));
    if(tmp_err != sizeof(validator_mq_msg)) {
        HANDLE_ERR_EXIT_WITH_MSG("RUN: Invalid read from pipe - word");
    }

    // perform validation
    prepared_msg.accepted = accept(&a, prepared_msg.word);

    // send completed message back to the validator
    validator_mq_send_msg(validator_mq, &prepared_msg, &err);
    HANDLE_ERR_EXIT_WITH_MSG("RUN: Failed to send message to validator mq");

    // clean up
    validator_mq_finish(false, validator_mq, &err);
    HANDLE_ERR_EXIT_WITH_MSG("RUN: Failed to close validator mq");
    return 0;
}
