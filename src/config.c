//
// Created by kowal on 13.01.18.
//

#include <stdio.h>
#include <stdarg.h>
#include "config.h"

void log_formatted(const char *fmt, ...) {
    if(enable_logging) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

void print_comm_summary(const comm_sumary_t *comm_summary, bool rcd_first) {
    if(rcd_first) {
        printf("Rcd: %zu\nSnt: %zu\nAcc: %zu\n", comm_summary->rcd, comm_summary->snt, comm_summary->acc);
    } else {
        printf("Snt: %zu\nRcd: %zu\nAcc: %zu\n", comm_summary->snt, comm_summary->rcd, comm_summary->acc);
    }
}
