//
// Created by kowal on 13.01.18.
//

#include <zconf.h>
#include <stdio.h>
#include <wait.h>
#include <stdlib.h>
#include "../src/err.h"

#define W_OPOR_DUZO 100000

struct test_str{
    char a[W_OPOR_DUZO];
    char b[W_OPOR_DUZO];
    char c[W_OPOR_DUZO];
};

void print_test_str(FILE *f, const struct test_str * x) {
    for(int i=0; i<W_OPOR_DUZO; i++) {
        fprintf(f, "%c %c %c\n", x->a[i], x->b[i], x->c[i]);
    }
}

int main() {
    int pipe_dsc[2];
    char pipe_read_dsc_str[10];

    struct test_str x;
    for(int i=0; i<W_OPOR_DUZO; i++) {
        char a, b, c;
        a = (char) (i % ('z' - 'a') + 'a');
        b = (char) ((i*i) % ('z' - 'a') + 'a');
        c = (char) ((i/W_OPOR_DUZO) * ('z' - 'a') + 'a');
        x.a[i] = a;
        x.b[i] = b;
        x.c[i] = c;
    }

    FILE *fp = fopen("test_result_parent.txt", "wb");
    if (!fp)
        syserr("Open error");
    print_test_str(fp, &x);
    fclose(fp);

    if (pipe(pipe_dsc) == -1) syserr("Error in pipe\n");

    switch (fork()) {
        case -1:
            syserr("Error in fork\n");

        case 0:
            if (close(pipe_dsc[1]) == -1) syserr("Error in close(pipe_dsc[1])\n");

            sprintf(pipe_read_dsc_str, "%d", pipe_dsc[0]);
            execl("./pipe_cpy_test_child", "pipe_cpy_test_child", pipe_read_dsc_str, NULL);
            syserr("Error in execl\n");

        default:
            if (close(pipe_dsc[0]) == -1) syserr("Error in close(pipe_dsc[0])\n");

            printf("Sending struct of size: %zu\n", sizeof(x));

            if (write(pipe_dsc[1], &x, sizeof(x)) != sizeof(x))
                syserr("Error in write\n");

            if (write(pipe_dsc[1], "\0", 1) != 1)
                syserr("Error in write\n");

            if (wait(0) == -1)
                syserr("Error in wait\n");

            exit(0);
    }
}
