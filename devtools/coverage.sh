#!/bin/sh

lcov --capture --directory coverage --output-file coverage/coverage.info
lcov --remove coverage/coverage.info '/usr/*' '*/tests/*' '*/testing/*' '*/third_party/*' '*/_deps/*' \
     --output-file coverage/coverage.info
genhtml coverage/coverage.info --output-directory coverage/html
