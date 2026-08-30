# Changelog

All notable changes to Hush Hush are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- JPEG carriers. `encode` and `decode` now accept a JPEG target and hide the
  payload in the low bits of its quantised DCT coefficients, with the same
  container, passphrase derived keys and scattered payload as the PNG path.
  The image is rewritten straight from its coefficients, so nothing is
  requantised.
- `assets/hush.jpg`, the JPEG counterpart of the sample carrier.

### Changed

- The container format (magic, sealed header, scatter walk, encryption) moved
  out of the LSB codec into `codecs/container.c`, which works against a
  `Carrier` of abstract bit slots. `codecs/lsb.c` supplies the pixel carrier and
  `codecs/dct.c` the coefficient one.
- libjpeg is a new build dependency.
