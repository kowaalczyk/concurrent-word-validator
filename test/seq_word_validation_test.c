//
// Created by kowal on 29.12.17.
//



// PASTE IMPLEMENTATION HERE -------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "../src/automaton.h"


/// checks if given state is universal in a given automata
bool is_universal(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
    return state < a->universal_states_size;
}

/// check if given state is existential in a given automata
bool is_existential(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
    return state >= a->universal_states_size;
}

/// iterates over acceptable states in automaton a, checking if the given state is acceptable, O(n)
bool is_acceptable(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
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
const char * get_following_states(const automaton * a, char state, char word_letter) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);
    assert(word_letter >= 'a');
    assert(word_letter <= 'z');

    state -= STR_STORAGE_VAL_OFFSET;
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
            size_t fs_len = strlen(following_state);
            // append one of possible following states to current state
            following_state[fs_len] = following_states[i];
            following_state[fs_len+1] = '\0';
//            strcat(following_state, &following_states[i]);
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
        size_t fs_len = strlen(following_state);
        // append one of possible following states to current state
        following_state[fs_len] = following_states[i];
        following_state[fs_len+1] = '\0';
//        strcat(following_state, &following_states[i]);
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
    char starting_state[STR_LEN_MAX];
    starting_state[0] = a->starting_state;
    starting_state[1] = '\0';
    return accept_rec(a, word, &starting_state);
}

// TEST ENGINE AUTOMATA LOAD IMPL --------------------------------------------------------------------------------------

/// loads automaton from string, delimited by endl char, allocates memory should be freed later
const automaton * test_load_data(const char * test_input_data) {
    // helpers for loading text instead of stdin
    char * test_loader = (char *) test_input_data;
    int test_loader_offset = 0;

    // iteration variables
    int i,j;
    // temporary data variables
    unsigned int n, a, u, q, f;
    int c_int;
    char c_char;
    int state;
    int letter;
    // buffers for loading arrays
    char * input_buff = NULL;
    int input_buff_offset = 0;
    size_t input_buff_len = 0;

    // allocate memory
    automaton * ans = (automaton*)malloc(sizeof(automaton));
    if(ans == NULL) {
        printf("Invalid malloc, cannot load automaton.\n");
        exit(1);
    }

    // clean arrays to prevent weird errors
    ans->acceptable_states[0] = '\0';
    for(j=0; j<TRANSITIONS_MAX_SIZE; j++) {
        ans->transitions[j][0] = '\0';
    }

    // load structure parameters
    sscanf(test_loader, "%d %n", &n, &test_loader_offset);
    test_loader += test_loader_offset;
    sscanf(test_loader, "%d %n", &a, &test_loader_offset);
    test_loader += test_loader_offset;
    sscanf(test_loader, "%d %n", &q, &test_loader_offset);
    test_loader += test_loader_offset;
    sscanf(test_loader, "%d %n", &u, &test_loader_offset);
    test_loader += test_loader_offset;
    sscanf(test_loader, "%d %n\n", &f, &test_loader_offset);
    test_loader += test_loader_offset;
    ans->alphabet_size = n;
    ans->states_size = q;
    ans->universal_states_size = u;

    // load structure arrays
    for(i=1; i<n; i++) {
        switch(i) {
            // 0 has already been loaded
            case 1:
                // loading starting state
                sscanf(test_loader, "%d\n%n", &c_int, &test_loader_offset);
                test_loader += test_loader_offset;

                assert(0 <= c_int && c_int < ans->states_size);
                ans->starting_state = (char) (c_int + STR_STORAGE_VAL_OFFSET); // offsetting states in strings, see automaton struct documentation
                break;
            case 2:
                // loading acceptable states
                for(j=0; j<f; j++) {
                    sscanf(test_loader, "%d\n%n", &c_int, &test_loader_offset);
                    test_loader += test_loader_offset;
                    assert(0 <= c_int && c_int < ans->states_size);
                    c_int += STR_STORAGE_VAL_OFFSET; // offsetting states in strings, see automaton struct documentation
                    ans->acceptable_states[j] = (char) c_int;
                }
                ans->acceptable_states[f] = '\0'; // Setting end of string manually
                break;
            default:
                // loading transition function
                assert(input_buff == NULL);
                assert(input_buff_offset == 0);
                assert(input_buff_len == 0);

                input_buff = test_loader; // no need to free anything in this version

                // Scanning state and letter separately in order to keep track of the offset
                sscanf(input_buff, "%d %n", &state, &input_buff_offset);
                input_buff += input_buff_offset;
                sscanf(input_buff, "%c %n", &c_char, &input_buff_offset);
                input_buff += input_buff_offset;
                assert(input_buff_offset > 0);

                // letter has to be normalized to fit in 0..ALPHABET_MAX_SIZE
                letter = (int)(c_char-'a');

                // reading transition function for given <state, letter>
                size_t transition_pos = state * (ans->alphabet_size) + letter;
                j = 0;
                while(sscanf(input_buff, "%d %n", &c_int, &input_buff_offset) == 1) {
                    c_int += STR_STORAGE_VAL_OFFSET; // offsetting states in strings, see automaton struct documentation
                    ans->transitions[transition_pos][j] = (char)(c_int);
                    j++;
                    input_buff += input_buff_offset;
                }
                ans->transitions[transition_pos][j] = '\0';

                // clean buffers
                input_buff = NULL;
                input_buff_offset = 0;
                input_buff_len = 0;
                break;
        }
    }
    return ans;
}

void test2() {
    const char autom[] = "7 2 2 1 1\n0\n0\n0 a 0 1\n0 b 0\n1 a 1\n1 b 0\0";
    const char words[4][100] = {
            "a\0",
            "\0",
            "ab\0",
            "aabbaba\0"
    };

    const automaton * a = test_load_data(autom);

    // TODO: Not sure if this assertions are correct at all, re-check before further testing
    assert(!accept(a, words[0]));
    assert(accept(a, words[1]));
    assert(accept(a, words[2]));
    assert(!accept(a, words[3]));

    free((void *) a);
}

int main() {
    test2();
    return 0;
}