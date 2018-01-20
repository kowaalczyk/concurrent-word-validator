//
// Created by kowal on 13.01.18.
//

#include <assert.h>
#include <stdio.h>
#include "config.h"

void print_comm_summary(const comm_sumary_t *comm_summary) {
    printf("Snt: %zu\nRcd: %zu\nAcc: %zu\n", comm_summary->snt, comm_summary->rcd, comm_summary->acc);
}
