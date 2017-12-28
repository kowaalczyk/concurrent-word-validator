//
// Created by kowal on 28.12.17.
//

// PASTE IMPLEMENTATION HERE -------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <wchar.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>

// NOTE: Using smaller data in order to make tests results visible
#define STR_LEN_MAX 5
#define ALPHABET_MAX_SIZE ('c'-'a')
#define STATES_MAX_SIZE 3
#define TRANSITIONS_MAX_SIZE (STATES_MAX_SIZE * ALPHABET_MAX_SIZE)

typedef struct automaton {
    size_t alphabet_size;
    size_t states_size; /// size of all states in automaton
    size_t universal_states_size; /// 0..universal_states_size-1 := universal states, universal_states_size..states_size-1 := existential states
    char starting_state; /// starting state for validation
    const char * acceptable_states; /// each char represents one state, states are indexed from 0
    const char * transitions[TRANSITIONS_MAX_SIZE]; /// transitions[state*(alphabet_size) + letter_normalized] := string containing all possible following states (as chars)
    size_t transitions_size; /// remembered for quicker iteration
} automaton;

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
    return a->transitions[state*ALPHABET_MAX_SIZE + word_letter];
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
    // need to accept_rec all of following states
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

// TEST ----------------------------------------------------------------------------------------------------------------


const automaton * test_load_auto() {
//    size_t alphabet_size, states_size, universal_states_size;
//
//
//    unsigned int n, a, u, q, f;
//    scanf("%d %d %d %d %d\n", &n, &a, &u, &q, &f);
//    char starting_state;
//
//    for(int i=0; i<n; i++) {
//        switch(i) {
//            case 0:
//                scanf("%c\n", &starting_state);
//                break;
//            case 1:
//
//        }
//    }
    return NULL;
}

int main() {
    automaton a = {
            ALPHABET_MAX_SIZE,
            STATES_MAX_SIZE,
            STATES_MAX_SIZE/2,
            'a',
            "zxyw",
            {"abc", "adef", "bghi", "ciz"},
            TRANSITIONS_MAX_SIZE
    };

    printf("Whole struct:\n");
    printf("%p\n", &a);
    printf("%zu\n", sizeof(a));
    printf("Single elements:\n");
    printf("%p\n", (void *) a.alphabet_size);
    printf("%zu\n", sizeof(a.alphabet_size));
    printf("%p\n", (void *) a.states_size);
    printf("%zu\n", sizeof(a.states_size));
    printf("%p\n", (void *) a.universal_states_size);
    printf("%zu\n", sizeof(a.universal_states_size));
    printf("%c\n", a.starting_state);
    printf("%zu\n", sizeof(a.starting_state));
    printf("%p\n", (void *) a.acceptable_states);
    printf("%zu\n", sizeof(a.acceptable_states));
    printf("%p\n", (void *) a.transitions);
    printf("%zu\n", sizeof(a.transitions));
    printf("Transitions:\n");
    for(size_t i=0; i<TRANSITIONS_MAX_SIZE; i++) {
        printf("%p ", &(a.transitions[i]));
    }
    printf("\nSize of 1 transition: %zu\n", sizeof(a.transitions[0]));
    printf("%p\n", (void *) a.transitions_size);
    printf("%zu\n", sizeof(a.transitions_size));

    return 0;
}