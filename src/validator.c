//
// Created by kowal on 27.12.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <mqueue.h>
#include <stdbool.h>
#include <zconf.h>
#include <wait.h>
#include "automaton.h"
#include "err.h"
#include "validator_queues.h"

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

void async_run_init(const char *word) {
    // TODO: Create MQ for sending automaton and word to run
    // TODO: Create run
    // TODO: Send automaton to run
    // TODO: Send word to run
    // TODO: Close MQ to run
}

void async_forward_response(const char * buffer, ssize_t buffer_size) {
    // TODO: Parse buffer if necessary
    // TODO: child process get response MQ mqd_t,
    // TODO: send response,
    // TODO: update local logs
}

int main() {
    const automaton * a = load_data();

    char * q_name = PW_VQ_REQUEST_NAME_PREFIX;
    mqd_t incoming_request_q = mq_open(q_name, O_RDONLY | O_CREAT);
    if(incoming_request_q == -1) {
        syserr("VALIDATOR: Failed to create requests queue");
    }

    ssize_t request_ret;
    char buffer[PW_VQ_REQUEST_BUFFSIZE];
    char word[STR_LEN_MAX];
    bool halt_flag_raised = false;
    size_t awaiting_runs = 0;
    size_t awaiting_forks = 0;
    while(!halt_flag_raised || awaiting_runs) {
        // wait for incoming message (from tester or run)
        request_ret = mq_receive(incoming_request_q, buffer, PW_VQ_REQUEST_BUFFSIZE, NULL);
        if(request_ret < 0) {
            syserr("VALIDATOR: Failed to receive request");
        }
        // process incoming message
        if(requested_halt(buffer, request_ret)) {
            halt_flag_raised = true;

        } else if(requested_validation_finish(buffer, request_ret)) {
            switch (fork()) {
                case -1:
                    syserr("VALIDATOR: Error in fork");
                case 0:
                    async_forward_response(buffer, request_ret);
                    break;
                default:
                    awaiting_forks++;
                    awaiting_runs--;
            }

        } else if(requested_validation_start(buffer, request_ret)) {
            switch (fork()) {
                case -1:
                    syserr("VALIDATOR: Error in fork");
                case 0:
                    // child creates run, sends automaton via mq, closes it and exits
                    get_request_word(buffer, request_ret, word);
                    async_run_init(word);
                    exit(0);
                default:
                    // parent will wait for all forks after all requests will be processed
                    // TODO: Create response MQ and open it for writing (this is non-blocking, tester already has an open queue)
                    awaiting_forks++;
                    awaiting_runs++;
            }
        }
    }
    // clean queue
    if (mq_close(incoming_request_q)) syserr("VALIDATOR: Failed to close queue");
    if (mq_unlink(q_name)) syserr("VALIDATOR: Failed to unlink queue");

    // clean memory
    free((void *) a);

    while(awaiting_forks > 0) {
        // TODO: Handle errors from forks
        wait(NULL);
        awaiting_forks--;
    }

    // TODO: Close all response MQs (if necessary)

    return 0;
}
