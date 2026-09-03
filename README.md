# Building

## Configure and build

```sh
cmake -S . -B build
cmake --build build -j
```

`hh` lands in `build/`, static libraries in `build/lib/`. The build type
defaults to `Release`; pass `-DCMAKE_BUILD_TYPE=Debug` for a debug build.

Sources and headers are globbed recursively per directory with
`CONFIGURE_DEPENDS`, so adding a file is enough, and the glob is re-run when the
file set changes. Headers sit next to the sources they belong to, there is no
mirrored `include/`.

`hushhush::core` links libsodium and libjpeg `PUBLIC`, so their headers come
along with it and the CLI does not restate them. Note that `jpeglib.h` is not
self contained: `stddef.h` and `stdio.h` have to be included ahead of it.

## Options

| Option                  | Default            | Effect                                |
| ----------------------- | ------------------ | ------------------------------------- |
| `HH_BUILD_CLI`          | `ON`               | Build the `hh` command line app       |
| `HH_BUILD_TESTS`        | `ON` at top level  | Build the test binaries and register them with CTest |
| `HH_WARNINGS_AS_ERRORS` | `OFF`              | `-Werror`, or `/WX` on MSVC           |
| `HH_ENABLE_COVERAGE`    | `OFF`              | Instrument for gcov and lcov, GCC or Clang only |

```sh
cmake -S . -B build -DHH_WARNINGS_AS_ERRORS=ON -DCMAKE_BUILD_TYPE=Debug
```

Warnings and the shared compile definitions live on one interface target,
`hushhush::options`, which everything else links, so a flag is set in one place.

## Tests

Tests live next to the code they cover, in `{module}/tests/` 
build into one binary each. The shared harness is in `testing/`: the vendored
[greatest](https://github.com/silentbicycle/greatest) header and the fixture
helpers, held by `hushhush::testing`. No test cases live there.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

or

```sh
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```

That runs `build/hh_{module}_tests`, registered with CTest

## Coverage

Requires `lcov`, and GCC or Clang:

```sh
cmake -S . -B coverage -DCMAKE_BUILD_TYPE=Debug -DHH_ENABLE_COVERAGE=ON
cmake --build coverage -j
ctest --test-dir coverage

./devtools/coverage.sh
```

The report lands in `coverage/html/index.html`.
