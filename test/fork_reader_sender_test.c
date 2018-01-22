//
// Created by kowal on 21.01.18.
//

#include <zconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <wait.h>

int main() {
    char buffer[1000];

    switch (fork()) {
        case -1:
            exit(EXIT_FAILURE);
        case 0:
            while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
                assert(buffer[strlen(buffer)-1] == '\n');

                buffer[strlen(buffer)-1] = '\0';
                printf("%d SND: %s\n", getpid(), buffer);
                buffer[0] = '\0'; // following shorter words cannot contain junk
            }
            break;
        default:
            close(0);
            wait(NULL);
            break;
    }
    return 0;
}