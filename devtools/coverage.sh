#!/bin/sh

lcov --capture --directory coverage --output-file coverage/coverage.info
lcov --remove coverage/coverage.info '/usr/*' '*/tests/*' '*/testing/*' '*/libs/*' \
     --output-file coverage/coverage.info
genhtml coverage/coverage.info --output-directory coverage/html
