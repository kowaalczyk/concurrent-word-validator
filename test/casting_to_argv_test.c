//
// Created by kowal on 22.01.18.
//

#include <stdio.h>
#include <zconf.h>

int main(int argc, char *argv[]) {
    pid_t parent_pid = -1;
    sscanf(argv[1], "%d", &parent_pid);
    printf("Received pid= %d\n", parent_pid);
}
