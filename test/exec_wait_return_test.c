//
// Created by kowal on 21.01.18.
//

#include <zconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>

int main() {
    // this now tests sending integer as argv too:
    pid_t parent_pid = getpid();
    char parent_pid_str[3* sizeof(pid_t)];
    sprintf(parent_pid_str, "%d", parent_pid);

    switch (fork()) {
        case -1:
            exit(-1);
        case 0:
            sleep(0); // just to allow a declaration in case body
            printf("Sending parent_pid=%d via execvp\n", parent_pid);
            char * child_argv[] = {"casting_to_argv_test", parent_pid_str, NULL};
            execvp("./casting_to_argv_test", child_argv);
            exit(-1);
        default:
            break;
    }

    int status;
    pid_t tmp_pid = wait(&status);
    printf("Fork %d returned: %d\n", tmp_pid, status);
    // (!!!) exec fork returned 0 => 0 cannot mean a word was sent in the validator

    return 0;
}
