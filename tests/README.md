# Tests

Host-side tests are available for utility functions.

Build and run (native GCC):
```bash
cd tests
gcc -I../Core/Inc ../Core/Src/utils.c utils_test.c -o utils_test
./utils_test
```

Note: these tests are intended to run on the host for quick validation of protocol helpers.
