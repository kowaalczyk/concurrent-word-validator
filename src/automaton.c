#include "automaton.h"
#include "config.h"
#include <stdlib.h>
#include <assert.h>

//
// Created by kowal on 29.12.17.
//
void load_automaton(automaton *ptr, bool *err) {
    VOID_FAIL_IF(ptr == NULL);

    // iteration variables
    unsigned int i,j;
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

    // clean arrays to prevent weird errors
    ptr->acceptable_states[0] = '\0';
    memset(ptr->acceptable_states, 0, STATES_SIZE);
    for(j=0; j<TRANSITIONS_SIZE; j++) {
        ptr->transitions[j][0] = '\0';
        memset(ptr->transitions[j], 0, STATES_SIZE);
    }

    // load structure parameters
    scanf("%d %d %d %d %d\n", &n, &a, &q, &u, &f);
    ptr->alphabet_size = a;
    ptr->states_size = q;
    ptr->universal_states_size = u;

    // load structure arrays
    for(i=1; i<n; i++) {
        switch(i) {
            // 0 has already been loaded
            case 1:
                // loading starting state
                scanf("%d\n", &c_int);
                assert(0 <= c_int && c_int < (int)ptr->states_size);

                ptr->starting_state = (char) (c_int + STR_STORAGE_VAL_OFFSET);
                break;
            case 2:
                // loading acceptable states
                for(j=0; j<f; j++) {
                    scanf("%d\n", &c_int);
                    assert(0 <= c_int && c_int < ptr->states_size);

                    c_int += STR_STORAGE_VAL_OFFSET;
                    ptr->acceptable_states[j] = (char) c_int;
                }
                ptr->acceptable_states[f] = '\0'; // Setting end of string manually
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
                size_t transition_pos = state * (ptr->alphabet_size) + letter;
                j = 0;
                while(sscanf(input_buff, "%d %n", &c_int, &input_buff_offset) == 1) {
                    c_int += STR_STORAGE_VAL_OFFSET; // offsetting states in strings, see automaton struct documentation
                    ptr->transitions[transition_pos][j] = (char)(c_int);
                    j++;
                    input_buff += input_buff_offset;
                }
                ptr->transitions[transition_pos][j] = '\0';

                // clean buffers
                free(input_buff_freeable);
                input_buff = NULL;
                input_buff_freeable = NULL;
                input_buff_offset = 0;
                input_buff_len = 0;
                break;
        }
    }
}
