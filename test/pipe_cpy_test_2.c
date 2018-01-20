//
// Created by kowal on 20.01.18.
//

//
// Created by kowal on 13.01.18.
//

#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <wait.h>
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

int main (int argc, char *argv[])
{
    ssize_t tmp_err;
    int pipe_dsc[2];
    struct test_str x;

    // create large structure to send
    for(int i=0; i<W_OPOR_DUZO-1; i++) {
        char a, b, c;
        a = (char) (i % ('z' - 'a') + 'a');
        b = (char) ((i*i) % ('z' - 'a') + 'a');
        c = (char) ((i/W_OPOR_DUZO) * ('z' - 'a') + 'a');
        x.a[i] = a;
        x.b[i] = b;
        x.c[i] = c;
    }
    x.a[W_OPOR_DUZO-1] = '\0';
    x.b[W_OPOR_DUZO-1] = '\0';
    x.c[W_OPOR_DUZO-1] = '\0';

    // create pipe
    tmp_err = pipe(pipe_dsc);
    if(tmp_err == -1) {
        syserr("Error in pipe");
    }

    switch (fork()) {
        case -1:
            syserr("error in fork");
        case 0:
            // child process chaneges descriptors and execs
            // TODO
            tmp_err = close(0);
            if(tmp_err) {
                syserr("Error in child: cannot close dsc 0");
            }
            tmp_err = dup(pipe_dsc[0]);
            if(tmp_err) {
                syserr("Error in child: cannot dup");
            }
            tmp_err = close(pipe_dsc[0]);
            if(tmp_err) {
                syserr("Error in child: cannot close pipe dsc 0");
            }
            tmp_err = close(pipe_dsc[1]);
            if(tmp_err) {
                syserr("Error in child: cannot close pipe dsc 1");
            }
            char * child_argv[2] = {"pipe_cpy_test_child_2", NULL};
            execvp("./pipe_cpy_test_child_2", child_argv);
            break;
        default:
            // parent sends message through the pipe
            tmp_err = close(pipe_dsc[0]);
            if(tmp_err == -1) {
                syserr("Error in parent: cannot close pipe dsc 0");
            }

            tmp_err = write(pipe_dsc[1], &x, sizeof(struct test_str));
            if(tmp_err != sizeof(struct test_str)) {
                syserr("Error in pipe write");
            }

            tmp_err = close(pipe_dsc[1]);
            if(tmp_err == -1) {
                syserr("Error in parent: cannot close pipe dsc 1");
            }
    }

    // print test struct
    FILE *fp = fopen("../test_2_result_parent.txt", "wb");
    if (!fp)
        syserr("Open error");
    print_test_str(fp, &x);
    fclose(fp);

    // wait for child to complete pipe operation
    if(wait(NULL) == -1) {
        syserr("Error in wait");
    }
    exit(0);
}
