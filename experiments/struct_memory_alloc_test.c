//
// Created by kowal on 28.12.17.
//


#include <stdbool.h>
#include <wchar.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// Set this to true for more info
const bool debug = false;

// PASTE IMPLEMENTATION HERE -------------------------------------------------------------------------------------------


// NOTE: Using smaller data in order to make tests results visible
#define STR_LEN_MAX 1000
#define ALPHABET_MAX_SIZE ('z'-'a')
#define STATES_MAX_SIZE 100
#define TRANSITIONS_MAX_SIZE (STATES_MAX_SIZE * ALPHABET_MAX_SIZE)

typedef struct automaton {
    size_t alphabet_size;
    size_t states_size; /// size of all states in automaton
    size_t universal_states_size; /// 0..universal_states_size-1 := universal states, universal_states_size..states_size-1 := existential states
    char starting_state; /// starting state for validation
    char acceptable_states[STATES_MAX_SIZE]; /// each char represents one state, states are indexed from 0
    char transitions[TRANSITIONS_MAX_SIZE][STATES_MAX_SIZE]; /// transitions[state*(alphabet_size) + letter_normalized] := string containing all possible following states (as chars)
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


automaton * test_load_auto() {
    // allocate memory
    automaton * ans = (automaton*)malloc(sizeof(automaton));
    if(ans == NULL) {
        printf("Invalid malloc, cannot load automaton.\n");
        exit(1);
    }

    // create temoprary arrays for loading data TODO: Load to structure directly
    char acceptable_states[STATES_MAX_SIZE];
    char transitions[TRANSITIONS_MAX_SIZE][STATES_MAX_SIZE];
    // clean arrays to prevent weird errors
    int i,j;
    for(i=0; i<STATES_MAX_SIZE; i++) {
        ans->acceptable_states[0] = '\0';
        acceptable_states[0] = '\0';
    }
    for(j=0; j<TRANSITIONS_MAX_SIZE; j++) {
        ans->transitions[j][0] = '\0';
        transitions[j][0] = '\0';
    }

    // load structure size parameters
    unsigned int n, a, u, q, f;
    scanf("%d %d %d %d %d\n", &n, &a, &u, &q, &f);
    ans->alphabet_size = n;
    ans->states_size = q;
    ans->universal_states_size = u;
    ans->transitions_size = 0;
    // load temporary arrays
    int c_int;
    char c_char;
    int state;
    int letter;
    char transition_buff[STATES_MAX_SIZE];
    transition_buff[0] = '\0';
    char * input_buff = NULL;
    char * input_buff_freeable = NULL;
    int input_buff_offset = 0;
    size_t input_buff_len = 0;
    for(i=1; i<n; i++) {
        switch(i) {
            // 0 has already been loaded
            case 1:
                // loading starting state
                scanf("%d\n", &c_int);
                assert(0 <= c_int <= CHAR_MAX);
                ans->starting_state = (char) c_int;
                break;
            case 2:
                // loading acceptable states
                for(j=0; j<f; j++) {
                    scanf("%d\n", &c_int);
                    assert(0 <= c_int <= CHAR_MAX);
                    acceptable_states[j] = (char) c_int;
                }
                acceptable_states[f] = '\0'; // Setting end of string manually
                break;
            default:
                // loading transition function
                assert(transition_buff[0] == '\0');
                assert(input_buff == NULL);
                assert(input_buff_offset == 0);
                assert(input_buff_len == 0);
                assert(input_buff_freeable == NULL);
                getline(&input_buff, &input_buff_len, stdin);
                input_buff_freeable = input_buff;

                // Scanning state and letter separately in order to keep track of the offset
                sscanf(input_buff, "%d %n", &state, &input_buff_offset);
                input_buff += input_buff_offset;
                sscanf(input_buff, "%c %n", &c_char, &input_buff_offset);
                input_buff += input_buff_offset;
                assert(input_buff_offset > 0);

                // letter has to be normalized to fit in 0..ALPHABET_MAX_SIZE
                letter = (int)(c_char-'a');
                if(debug) {
                    printf("\nLine: %d State: %d Letter: %d\n", i, state, letter);
                }

                // reading transition function for given <state, letter>
                j = 0;
                while(sscanf(input_buff, "%d %n", &c_int, &input_buff_offset) == 1) {
                    transition_buff[j] = (char)(c_int);
                    if(debug) {
                        printf("Saved: %d at: %d\n", (int)transition_buff[j], j);
                    }
                    j++;
                    input_buff += input_buff_offset;
                }
                transition_buff[j] = '\0';
                strcpy(transitions[state*(ans->alphabet_size) + letter], transition_buff);
                ans->transitions_size++;

                // clean buffers
                transition_buff[0] = '\0';
                free(input_buff_freeable);
                input_buff = NULL;
                input_buff_freeable = NULL;
                input_buff_offset = 0;
                input_buff_len = 0;
                break;
        }
    }
    // copy loaded values into automaton
    strcpy(ans->acceptable_states, acceptable_states);
    for(int i=0; i<ans->transitions_size; i++) {
        strcpy(ans->transitions[i], transitions[i]);
        if(debug) {
            printf("successfully copied %d to structure\n", i);
        }
    }
    return ans;
}

void test_print_mem_data(const automaton * a) {
    printf("Whole struct:\n");
    printf("%p\n", a);
    printf("%zu\n", sizeof(*a));
    printf("Single elements:\n");
    printf("%p\n", (void *) a->alphabet_size);
    printf("%zu\n", sizeof(a->alphabet_size));
    printf("%p\n", (void *) a->states_size);
    printf("%zu\n", sizeof(a->states_size));
    printf("%p\n", (void *) a->universal_states_size);
    printf("%zu\n", sizeof(a->universal_states_size));
    printf("%c\n", a->starting_state);
    printf("%zu\n", sizeof(a->starting_state));
    printf("%p\n", (void *) a->acceptable_states);
    printf("%zu\n", sizeof(a->acceptable_states));
    printf("%p\n", (void *) a->transitions);
    printf("%zu\n", sizeof(a->transitions));
    printf("%p\n", (void *) a->transitions_size);
    printf("%zu\n", sizeof(a->transitions_size));
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

    automaton * b = test_load_auto();

    test_print_mem_data(&a);
    test_print_mem_data(b);

    free(b);
    return 0;
}