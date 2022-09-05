# Contributing to the repository

Any proposed change must linked to an open issue. Use the issue template to open a new issue if required.

Changes are submitted via merge requests and require review and approval by the project maintainers before being merged.

## Minimum environment support
Features and fixes must support the following requirements
* C++ 11 compatibility
* compatible with gcc and clang
* Linux x86-64

## Requirements
* a C++ compiler, naturally
* clang-tidy 15+
* cmake 3.21+

## Testing and benchmarking
`dxpool` is covered by unit and fuzzing tests, executed automatically on pushes and MRs.
Features and fixes must include sufficient unit tests and, if required, fuzzing tests and new benchmarks. Use good judgement deciding when fuzzing tests are required and follow maintainers recommendations.

All classes and public interfaces must be documented (doxygen style).