#!/usr/bin/env bash

cat ./test_data/test1.in | ./cmake-build-debug/seq_word_validation_test
cat ./test_data/test2.in | ./cmake-build-debug/seq_word_validation_test
cat ./test_data/test3.in | ./cmake-build-debug/seq_word_validation_test
cat ./test_data/test4.in | ./cmake-build-debug/seq_word_validation_test
