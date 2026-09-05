# Changelog

All notable changes to Hush Hush are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `hh-gui`, a desktop inspector. 

### Changed

- `analysis/inspect.h` now takes a `PixelBuffer` and returns the recovered
  bytes packed into an `LsbStream`, with `InspectRow` filled in place by the
  caller rather than allocated per byte. A megapixel carrier is millions of
  rows, and three allocations each was more than a viewer could carry.
  `hh inspect` reads the same way and prints the same output.
- Opening a JPEG and walking its coefficients moved into
  `handlers/jpeg.c`. `codecs/dct.c` and `analysis/dct.c` had grown their own
  copies of the walk and of the rule deciding which coefficients carry a bit,
  which is exactly the rule the two must never disagree on.

### Fixed

- LSB inspection read the alpha channel, which the codec never writes to, so
  the recovered stream was wrong for any RGBA carrier. It now skips alpha the
  way `codecs/lsb.c` does.
- `hh inspect` leaked its rows and the decoded image on every run, and its
  pixel buffer again on the failure path.

## [0.1.1]

### Added

- JPEG carriers. `encode` and `decode` now accept a JPEG target and hide the
  payload in the low bits of its quantised DCT coefficients, with the same
  container, passphrase derived keys and scattered payload as the PNG path.
  The image is rewritten straight from its coefficients, so nothing is
  requantised.

### Changed

- The container format (magic, sealed header, scatter walk, encryption) moved
  out of the LSB codec into `codecs/container.c`, which works against a
  `Carrier` of abstract bit slots. `codecs/lsb.c` supplies the pixel carrier and
  `codecs/dct.c` the coefficient one.
