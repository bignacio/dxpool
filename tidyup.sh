#!/bin/sh

clang-tidy --config-file=clang-tidy.config -p build $@

#clang-tidy -checks='-*,readability-*,clang-analyzer-core.*,clang-analyzer-cplusplus.*,modernize-*,performance-*,concurrency-*,cppcoreguidelines-*,bugprone*' \
#     --warnings-as-errors=* -header-filter=*.\(?!build\).* -p build $@