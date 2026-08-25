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

---

## Table of Contents

- [About](#about)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
  - [Examples](#examples)
- [Development](#development)
  - [Tests](#tests)
  - [Coverage](#coverage)
  - [Formatting](#formatting)
  - [Devtools](#devtools)
  - [Editor Integration](#editor-integration)
- [Contributing](#contributing)
- [License](#license)

---

## About

Hush Hush hides one file inside another. For example give it a PNG and a payload, and it
writes out a PNG that looks exactly like the one you handed it, carrying your
bytes in the low bits of its pixels.

Supply a passphrase and the payload is sealed with XSalsa20-Poly1305 before it
goes in.

Leave the passphrase empty and the payload goes in as-is, sequentially,
terminated by a marker. That mode is for convenience, not for secrecy.

## Requirements

Linux, plus the following:

| Dependency  | Used for                                        |
| ----------- | ----------------------------------------------- |
| libsodium   | Key derivation, secretbox, ChaCha20, memzero    |
| pkg-config  | Locating libsodium at build time                |
| GNU ld      | The `commands` section the subcommand table lives in |

Debian and Ubuntu:

```sh
sudo apt install build-essential pkg-config libsodium-dev
```

Fedora:

```sh
sudo dnf install gcc make pkgconf-pkg-config libsodium-devel
```

Arch:

```sh
sudo pacman -S base-devel pkgconf libsodium
```

Optional: `clang-format` for the formatting targets, `lcov` for coverage, and
Python with Pillow for the image comparison devtool.

## Building

```sh
make
```

That produces the `hh` binary in the project root and regenerates
`compile_commands.json`. Sources and headers are discovered recursively, so new
files under `src/` and `include/` are picked up without editing the Makefile.

Subcommands register themselves at link time: `COMMAND(cmd)` places a pointer
into the `commands.*` section, `linker.ld` gathers those between
`__start_commands` and `__stop_commands`, and `find_command` binary searches the
result. Adding a subcommand means adding a file, nothing else.

```sh
make clean
```

## Usage

```
./hh [--verbose] <subcommand> <target> [options]
```

`--verbose` comes before the subcommand and turns on the timestamped debug log.
Only PNG carriers are supported; the type is decided by the file's magic bytes,
not by its extension.

### Examples

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

Watch what the encoder is doing while it works. Leaving the passphrase empty at
the prompt writes a plain container instead of an encrypted one:

```sh
./hh --verbose encode photo.png notes.txt -o innocent.png
```

Read the raw low-bit stream of an image, one byte per line, as binary, hex and
ASCII:

```sh
./hh inspect innocent.png -l 20
```

`inspect` prints the first `-l` lines when it is writing to a terminal, and
everything when it is not, so pipe it to a file to read the whole stream:

```sh
./hh inspect innocent.png > bits.txt
```

## Development

### Tests

```sh
make test
```

Every production object except `main.o` is linked into the test runner, which is
built on the vendored [greatest](https://github.com/silentbicycle/greatest)
header.

### Coverage

Requires `lcov`:

```sh
make coverage
```

The HTML report lands in `coverage/html/index.html`.

### Formatting

```sh
make format        # rewrite sources in place
make format-check  # fail if anything is unformatted
```

The style is in `.clang-format`: 4 spaces, 125 columns, pointers bound to the
name.

### Devtools

```sh
./devtools/checksum.sh original.pdf recovered.pdf
```

Compares two files by SHA-256, which is how a round trip is confirmed to be
lossless.

```sh
python3 devtools/compare_image.py carrier.png output.png
```

Reports how many pixels the encoder touched and by how much, which is the quick
way to see the difference between `lsbr` and `lsbm`.

### Editor Integration

`make` regenerates `compile_commands.json`, so `clangd` picks up the include
paths and the libsodium flags with no extra configuration. `.clangd`,
`.editorconfig` and `.clang-format` are checked in.

## Contributing

Pull requests are welcome. Every pull request needs a `CHANGELOG.md` entry, or
the `ci/skip-changelog` label if the change does not deserve one, and CI builds
and tests the tree with both GCC and Clang before it can merge. Bug reports and
feature requests have their own issue forms.
