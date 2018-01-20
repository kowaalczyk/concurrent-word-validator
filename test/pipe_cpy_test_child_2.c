//
// Created by kowal on 20.01.18.
//

#include <zconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
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
    struct test_str x;
    char tmp[1000];
    ssize_t tmp_err;

    tmp_err = read(0, &x, sizeof(struct test_str));
    assert(tmp_err == sizeof(struct test_str));
    // print test struct
    FILE *fp = fopen("../test_2_result_child.txt", "wb");
    if (!fp)
        syserr("Open error");
    print_test_str(fp, &x);
    fclose(fp);

    tmp_err = read(0, tmp, sizeof(tmp));
    assert(tmp_err == strlen("loremipsum")+1);
    printf("%s\n", tmp);
    assert(!strcmp(tmp, "loremipsum"));
}
