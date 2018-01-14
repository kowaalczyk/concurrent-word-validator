//
// Created by kowal on 13.01.18.
//

#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
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
    int read_dsc;
    ssize_t buf_len;

    struct test_str x;

    if (argc != 2)
        fatal("Usage: %s <read_fd>\n", argv[0]);

    read_dsc = atoi(argv[1]);
    printf("Reading data from descriptor %d\n", read_dsc);

    if ((buf_len = read(read_dsc, &x, sizeof(x)-1)) == -1)
        syserr("Error in read\n");;


    if (buf_len == 0)
        fatal("Unexpected end-of-file\n");
    else {
        printf("Received struct of size: %zu\n", buf_len);
        printf("--- RECEIVED STRUCT --- \n");

        FILE *fp = fopen("test_result_child.txt", "wb");
        if (!fp)
            syserr("Open error");
        print_test_str(fp, &x);
        fclose(fp);
    }
    exit(0);
}
