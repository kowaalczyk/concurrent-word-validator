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
    return accept_rec(a, word, starting_state);
}

// TEST ENGINE AUTOMATA LOAD IMPL --------------------------------------------------------------------------------------


/// loads automaton from standard input, allocates memory should be freed later
const automaton * load_data() {
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
    char * input_buff_freeable = NULL;
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
    scanf("%d %d %d %d %d\n", &n, &a, &q, &u, &f);
    ans->alphabet_size = n;
    ans->states_size = q;
    ans->universal_states_size = u;

    // load structure arrays
    for(i=1; i<n; i++) {
        switch(i) {
            // 0 has already been loaded
            case 1:
                // loading starting state
                scanf("%d\n", &c_int);
                assert(0 <= c_int && c_int < ans->states_size);
                ans->starting_state = (char) (c_int + STR_STORAGE_VAL_OFFSET); // offsetting states in strings, see automaton struct documentation
                break;
            case 2:
                // loading acceptable states
                for(j=0; j<f; j++) {
                    scanf("%d\n", &c_int);
                    assert(0 <= c_int && c_int < ans->states_size);
                    c_int += STR_STORAGE_VAL_OFFSET; // offsetting states in strings, see automaton struct documentation
                    ans->acceptable_states[j] = (char) c_int;
                }
                ans->acceptable_states[f] = '\0'; // Setting end of string manually
                break;
            default:
                // loading transition function
                assert(input_buff == NULL);
                assert(input_buff_freeable == NULL);
                assert(input_buff_offset == 0);
                assert(input_buff_len == 0);
                if(getline(&input_buff, &input_buff_len, stdin) == -1) {
                    printf("Failed to perform getline() during automaton load: error in stdin or during memory allocation.\n");
                    exit(1);
                }
                input_buff_freeable = input_buff; // input_buff is incremented, this is used to free memory later

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
                free(input_buff_freeable);
                input_buff = NULL;
                input_buff_freeable = NULL;
                input_buff_offset = 0;
                input_buff_len = 0;
                break;
        }
    }
    return ans;
}


/**
 * Runs single test, requires input from stdin:
 * [automaton description]
 * [number of words to process]
 * [word 1|0]
 * where 1 means the given word should be accepted, 0 that it should not.
 */
void run_test() {
    const automaton * a = load_data();
    int i, num_of_words;

    scanf("%d", &num_of_words);
    int x = num_of_words; // for some reason num_of_words is nulled after each iteration, TODO: FIX
    for(i = 0; i < x; i++) {
        printf("Running test case #%d of %d:\n", i+1, num_of_words);
        char word[STR_LEN_MAX];
        bool expected_ans;

        scanf("%s", word);
        scanf("%d", &expected_ans);
        assert(expected_ans==0 || expected_ans == 1);

        bool ans = accept(a, word);
        if(ans != expected_ans) {
            printf("FAILED TEST: %s - ", word);
            char boolstr[2][100] = {"false", "true"};
            printf("got %s instated of %s. \n", boolstr[ans], boolstr[expected_ans]);
        } else {
            printf("PASSED: %s\n", word);
        }
    }
    printf("Finished: %d %d.\n", i, x);
    free((void *) a);
}

int main() {
    run_test();
    return 0;
}