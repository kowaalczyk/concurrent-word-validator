//
// Created by kowal on 21.01.18.
//

#include <zconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>

int main() {

    switch (fork()) {
        case -1:
            exit(-1);
        case 0:
            sleep(0); // just to allow a declaration in case body
            char * child_argv[] = {"seq_word_validation_test", NULL};
            execvp("./tester_mq_test", child_argv);
            exit(-1);
        default:
            break;
    }

    int status;
    pid_t tmp_pid = wait(&status);
    printf("Fork %d returned: %d\n", tmp_pid, status);
    // exec fork returned 0 => 0 cannot mean a word was sent in the validator

    return 0;
}