//
// Created by kowal on 13.01.18.
//

#include <assert.h>
#include <stdio.h>
#include "config.h"

void pidstr(pid_t pid, char *target) {
    assert(sizeof(target) == PID_STR_LEN);

    sprintf(target, "%0*d", (int)PID_STR_LEN, pid);
}
