//
// Created by kowal on 29.12.17.
//



// PASTE IMPLEMENTATION HERE -------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <wait.h>
#include "../src/config.h"
#include "../src/automaton.h"


///// checks if given state is universal in a given automata
//static bool is_universal(const automaton * a, char state) {
//    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
//    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);
//
//    state -= STR_STORAGE_VAL_OFFSET;
//    return state < (int)a->universal_states_size;
//}

/// check if given state is existential in a given automata
static bool is_existential(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    state -= STR_STORAGE_VAL_OFFSET;
    return state >= (int)a->universal_states_size;
}

/// iterates over acceptable states in automaton a, checking if the given state is acceptable, O(n)
static bool is_acceptable(const automaton * a, char state) {
    assert(state >= 0 + STR_STORAGE_VAL_OFFSET);
    assert(state < a->states_size + STR_STORAGE_VAL_OFFSET);

    size_t acceptable_states_length = strlen(a->acceptable_states);
    int i;
    for(i=0; i<(int)acceptable_states_length; i++) {
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

    char state_list_extended[WORD_LEN_MAX+2];
    strcpy(state_list_extended, state_list);
    size_t sle_len = strlen(state_list_extended);

    if(is_existential(a, state_list[depth])) {
        // need to accept_rec any of following states
        if(following_states_length == 0) {
            return (depth < w_len);
        }

        bool ans = false;
        bool err = false;
        size_t await_forks = 0;
        int i;
        for(i=0; i<(int)following_states_length-1; i++) {
            switch (fork()) {
                case -1:
                    err = true;
                    HANDLE_ERR_EXIT_WITH_MSG("RUN: Failed to perform fork");
                case 0:
                    // append one of possible following states to current state_list
                    state_list_extended[sle_len] = following_states[i];
                    state_list_extended[sle_len+1] = '\0';
                    assert(strlen(state_list_extended) == strlen(state_list)+1);
                    if(accept_rec(a, word, state_list_extended)) {
                        exit(EXIT_SUCCESS);
                    }
                    exit(EXIT_FAILURE);

                default:
                    await_forks++;
            }
        }
        assert(i == following_states_length-1);
        // append last of possible following states to current_stat_list and check it in current process
        state_list_extended[sle_len] = following_states[i];
        state_list_extended[sle_len+1] = '\0';
        assert(strlen(state_list_extended) == strlen(state_list)+1);

        if(accept_rec(a, word, state_list_extended)) {
            ans = true;
        }

        while(await_forks) {
            int tmp_err = 0;
            int tmp_ret = 0;
            tmp_err = wait(&tmp_ret);
            if(tmp_err == -1) {
                err = true;
            }
            if(!WIFEXITED(tmp_ret)) {
                err = true;
            } else {
                int wait_ret = WEXITSTATUS(tmp_ret);
                ans = ans || (wait_ret == EXIT_SUCCESS);
            }
            await_forks--;
        }
        HANDLE_ERR_EXIT_WITH_MSG("RUN: Unexpected error in wait");
        return ans;
    }
    // need to accept all of following states
    if(following_states_length == 0) {
        return (depth < w_len);
    }

    bool ans = false;
    bool err = false;
    size_t await_forks = 0;
    int i;
    for(i=0; i<(int)following_states_length-1; i++) {
        switch (fork()) {
            case -1:
                err = true;
                HANDLE_ERR_EXIT_WITH_MSG("RUN: Failed to perform fork");
            case 0:
                // append one of possible following states to current state_list
                state_list_extended[sle_len] = following_states[i];
                state_list_extended[sle_len+1] = '\0';
                assert(strlen(state_list_extended) == strlen(state_list)+1);
                if(accept_rec(a, word, state_list_extended)) {
                    exit(EXIT_SUCCESS);
                }
                exit(EXIT_FAILURE);

            default:
                await_forks++;
        }
    }
    assert(i == following_states_length-1);
    // append last possible following state and check it in current process
    state_list_extended[sle_len] = following_states[i];
    state_list_extended[sle_len+1] = '\0';
    assert(strlen(state_list_extended) == strlen(state_list)+1);

    if(accept_rec(a, word, state_list_extended)) {
        ans = true;
    }

    while(await_forks) {
        int tmp_err = 0;
        int tmp_ret = 0;
        tmp_err = wait(&tmp_ret);
        if(tmp_err == -1) {
            err = true;
        }
        if(!WIFEXITED(tmp_ret)) {
            err = true;
        } else {
            int wait_ret = WEXITSTATUS(tmp_ret);
            ans = ans && (wait_ret == EXIT_SUCCESS);
        }
        await_forks--;
    }
    HANDLE_ERR_EXIT_WITH_MSG("RUN: Unexpected error in wait");
    return ans;
}

/// checks if given automaton accepts given word
static bool accept(const automaton * a, const char * word) {
    char state_list[WORD_LEN_MAX+2]; // states list for given word is equal its length
    state_list[0] = a->starting_state;
    state_list[1] = '\0';
    return accept_rec(a, word, state_list);
}

// TEST ENGINE AUTOMATA LOAD IMPL --------------------------------------------------------------------------------------

/**
 * Runs single test, requires input from stdin:
 * [automaton description]
 * [word\n[1|0]\n]
 * where 1 means the given word should be accepted, 0 that it should not.
 */
void run_test() {
    automaton a;
    size_t failed = 0;
    bool err = false;
    void *tmp_err = 0;
    load_automaton(&a, &err);

    char buffer[2*WORD_LEN_MAX]; // + '\n' and '\0'
    while(fgets(buffer, sizeof(buffer), stdin)) {
        assert(strlen(buffer) < WORD_LEN_MAX +2);
        assert(buffer[strlen(buffer)-1] == '\n');
        buffer[strlen(buffer)-1] = '\0';

        // load answer for loaded word
        char expected_ans_tmp[10];
        tmp_err = fgets(expected_ans_tmp, 10, stdin);
        err = (tmp_err == NULL);
        bool expected_ans = (bool)(expected_ans_tmp[0]-'0');
        assert(expected_ans==true || expected_ans == false);

        // testing validation result
        bool ans = accept(&a, buffer);
        if(ans != expected_ans) {
            failed++;
            printf("FAILED TEST: %s - ", buffer);
            char boolstr[2][100] = {"false", "true"};
            printf("got %s instead of %s. \n", boolstr[ans], boolstr[expected_ans]);
        } else {
            printf("PASSED: %s\n", buffer);
        }
        printf("\n");
        buffer[0] = '\0';
    }
    printf("Finished, # of failed cases: %zu.\n", failed);
}

int main() {
    run_test();
    return 0;
}