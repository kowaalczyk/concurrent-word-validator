#!/usr/bin/env bash

echo "NOTE: Grep will return exit status 1 if all tests are passed."
cat ./test_data/test1.in | ./cmake-build-debug/seq_word_validation_test | grep FAIL
cat ./test_data/test2.in | ./cmake-build-debug/seq_word_validation_test | grep FAIL
cat ./test_data/test3.in | ./cmake-build-debug/seq_word_validation_test | grep FAIL
cat ./test_data/test4.in | ./cmake-build-debug/seq_word_validation_test | grep FAIL
