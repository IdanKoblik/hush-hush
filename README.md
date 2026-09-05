<div align="center">

<img src="assets/hush.png" alt="Hush Hush" width="420">

<h1>Hush Hush</h1>

> Nothing to see here. Really.

[![CI](../../actions/workflows/ci.yml/badge.svg)](../../actions/workflows/ci.yml)
[![Language](https://img.shields.io/badge/language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#requirements)
[![Crypto](https://img.shields.io/badge/crypto-libsodium-green.svg)](https://doc.libsodium.org/)
[![License](https://img.shields.io/badge/license-GPL--3.0-orange.svg)](LICENSE)

</div>

# Installation

## From a release

Each [release](https://github.com/IdanKoblik/hush-hush/releases) ships loose
assets per architecture, `x86_64` and `aarch64`: the `hh` CLI, the `hh-gui`
desktop app and the `libhushhush.a` static library, alongside a single
`checksums.txt` covering all of them. Nothing to unpack — download the one you
want. Both architectures are built on Ubuntu 22.04, against the oldest glibc the
runners offer, so they run on most distributions.

```sh
VERSION=0.1.1
SUFFIX="${VERSION}-linux-$(uname -m)"
BASE="https://github.com/IdanKoblik/hush-hush/releases/download/v${VERSION}"

curl -LO "$BASE/hh-$SUFFIX"
curl -LO "$BASE/checksums.txt"

sha256sum --ignore-missing -c checksums.txt
chmod +x "hh-$SUFFIX"
```

`hh-gui-$SUFFIX` and `libhushhush-$SUFFIX.a` are fetched the same way. The
headers are not published as assets: to build against the library, take them
from `core/` in the source tree at the matching tag.

## From source

```sh
git clone https://github.com/IdanKoblik/hush-hush.git
cd hush-hush

cmake -S . -B build
cmake --build build -j
sudo cmake --install build
```

That installs `hh` into `CMAKE_INSTALL_PREFIX/bin`, `/usr/local/bin` by default.
Pass `--prefix ~/.local` to install without `sudo`. Only the CLI has an install
rule; to use the library, build it and point at `build/lib/libhushhush.a` and
the headers in `core/` where they sit.

Skipping the install step is fine too: the binary runs from `build/hh`. See
[Building](#building) for the options, the test suite and coverage.

# Usage

```
./hh [--verbose] <subcommand> <target> [options]
```

`--verbose` comes before the subcommand and turns on the timestamped debug log.
PNG and JPEG carriers are supported; the type is decided by the file's magic
bytes, not by its extension, and it picks the codec: LSB over pixels for PNG,
LSB over DCT coefficients for JPEG. `-c` selects how a bit is written in either
case, and a JPEG holds noticeably less than a PNG of the same size because only
non-trivial AC coefficients are usable.

## Examples

Hide a document in a photo, encrypted:

```sh
./hh encode photo.png secrets.pdf -o innocent.png
Passphrase (leave empty to disable encryption):
[+] Image: 1920x1080, 4 channels
[+] Capacity: 777600 bytes
[+] Data to encode: 41231 bytes
[+] Encryption: on
[+] Encoded successfully -> innocent.png
```

Take it back out:

```sh
./hh decode innocent.png -o secrets.pdf
Passphrase (leave empty if the data is not encrypted):
[+] Container: version 1, encryption on
[+] Decoded 41231 bytes from innocent.png
[+] Decoded successfully -> secrets.pdf
```

Use LSB matching instead of replacement:

```sh
./hh encode photo.png notes.txt -o innocent.png -c lsbm
```

Same thing with a JPEG carrier:

```sh
./hh encode photo.jpg secrets.pdf -o innocent.jpg
Passphrase (leave empty to disable encryption):
[+] Image: 567x440, 3 channels
[+] Capacity: 5095 bytes
[+] Data to encode: 2048 bytes
[+] Encryption: on
[+] Encoded successfully -> innocent.jpg
```

Watch what the encoder is doing while it works. Leaving the passphrase empty at
the prompt writes a plain container instead of an encrypted one:

```sh
./hh --verbose encode photo.png notes.txt -o innocent.png
```

Score how much a carrier looks like it is hiding something. Nothing is
decoded: every method scores how far the carrier is from one whose low bits are
random, which is what a payload leaves behind.

```sh
./hh analyse innocent.png
[+] Target: innocent.png
[+] Type: PNG, 1920 x 1080, 3 channels

  Method                        Measured Random bits  Population
  Low bit ratio                  49.94 %        50 %     6220800 colour samples
  Chi-square (PoV)               87.31 %       100 %         128 pairs of values
  Histogram of differences       49.21 %        50 %     6217920 neighbour pairs
  DCT histogram (PoV)                n/a
```

Read the table by how close **Measured** sits to **Random bits**, never as a
verdict on its own: a photograph of noise looks embedded and a lightly filled
carrier looks clean. The four methods are

- **Low bit ratio**, the codec's own rule read backwards: the share of colour
  samples whose low bit is set, `n1 / (n0 + n1)`. The coarsest of the four.
- **Chi-square (PoV)**, Westfeld and Pfitzmann's pairs of values test. Writing
  a bit moves a sample inside its pair, `2k` and `2k + 1`, and never out of it,
  so a payload drives the two counts of a pair together while leaving their sum
  alone. The percentage is the probability that the histogram is the one
  embedding would have left.
- **Histogram of differences**, the same ratio over `d = s[x + 1] - s[x]`
  instead of over samples. The parity of a difference is one sample's low bit
  xor the other's, so neighbours that agree, as they do all over a photograph,
  hold the share below half; random low bits pull it to half.
- **DCT histogram (PoV)**, the pairs of values test over the quantised
  coefficients, where a JPEG carrier hides its bits. `n/a` on a PNG.

`-e` prints what each one measures underneath the table:

```sh
./hh analyse innocent.png -e
```

`hh-gui` shows the same four numbers under its **Analysis** tab, alongside the
histograms they came out of.

# Editor Integration

Configuring the build regenerates `compile_commands.json` and links it into the
source root, so `clangd` picks up the include paths and the libsodium flags with
no extra configuration. `.clangd`, `.editorconfig` and `.clang-format` are
checked in.

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
| `HH_BUILD_GUI`          | `OFF`              | Build the `hh-gui` desktop app        |
| `HH_BUILD_TESTS`        | `ON` at top level  | Build the test binaries and register them with CTest |
| `HH_WARNINGS_AS_ERRORS` | `OFF`              | `-Werror`, or `/WX` on MSVC           |
| `HH_ENABLE_COVERAGE`    | `OFF`              | Instrument for gcov and lcov, GCC or Clang only |

```sh
cmake -S . -B build -DHH_WARNINGS_AS_ERRORS=ON -DCMAKE_BUILD_TYPE=Debug
```

Warnings and the shared compile definitions live on one interface target,
`hushhush::options`, which everything else links, so a flag is set in one place.

## GUI

`gui/` is the one C++17 directory in an otherwise C17 tree, and it is off by
default: it needs GLFW and a GL stack installed (`libglfw3-dev` and
`libgl1-mesa-dev` on Debian and Ubuntu), and it fetches
[Dear ImGui](https://github.com/ocornut/imgui) and
[ImPlot](https://github.com/epezent/implot) at configure time, both pinned via
`HH_IMGUI_TAG` and `HH_IMPLOT_TAG`.

```sh
cmake -S . -B build -DHH_BUILD_GUI=ON
cmake --build build -j
./build/hh-gui
```

Every header under `core/` carries an `extern "C"` guard, placed after its own
includes so that only core's declarations are wrapped and not the system headers
they pull in. `<core/...>` is therefore included directly from C++, with nothing
to remember at the call site.

`implot_items.cpp` dominates a first build; it is one vendored translation unit
compiled once, and nothing in `gui/` invalidates it afterwards. ImGui's and
ImPlot's demo galleries are left out of the build for the same reason, so
`ShowDemoWindow()` is unavailable until you add those two files back to
`hh_imgui` in `gui/CMakeLists.txt`.

Only the parts of the GUI that hold no ImGui state are unit tested, in
`gui/tests/`; the suite opens no window and so runs headless like the other two.

## Tests

Tests live next to the code they cover, in `{module}/tests/` 
build into one binary each, registered with CTest as `core`, `cli` and `gui`
(the last only when `HH_BUILD_GUI` is on). The shared harness is in `testing/`: the vendored
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

# Contributing

Pull requests are welcome. Every pull request needs a `CHANGELOG.md` entry, or
the `ci/skip-changelog` label if the change does not deserve one, and CI builds
and tests the tree with both GCC and Clang before it can merge. Bug reports and
feature requests have their own issue forms.
