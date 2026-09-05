# web

`index.html` is the whole app: one page, htmx, and the C core compiled to
WebAssembly. Nothing is uploaded anywhere — carriers are written into
Emscripten's in-memory file system, handed to the same functions `hh` calls,
and read back out.

## Build

Needs `emcc` on PATH. Everything else is fetched into `web/.deps` and nothing
is installed system wide.

```sh
./web/build.sh
python3 -m http.server -d web 8000
```

Then open <http://localhost:8000>. It has to be served over HTTP; a `file://`
URL cannot load the wasm module.

`./web/build.sh clean` throws away `.deps`, `.build` and `dist`.

## What is here

| Path            | What it is                                              |
| --------------- | ------------------------------------------------------- |
| `index.html`    | The page. Markup, styles, router and bridge, self contained. |
| `wasm/hh_web.c` | The six entry points the page calls, over `core/`.      |
| `wasm/smoke.mjs`| A round trip through the built module under node.       |
| `dist/`         | Build output, `hushhush.mjs` and `hushhush.wasm`.       |

After a build, `node web/wasm/smoke.mjs` encodes and decodes both carrier types,
plain and encrypted, and checks the bytes come back unchanged.

## How htmx talks to wasm

The page is ordinary htmx — `hx-post`, `hx-target`, `hx-swap-oob`, no custom
attributes. The requests it makes are answered locally instead of over the
network.

An extension listens for `htmx:beforeRequest`, which htmx fires before it
touches the network or the request indicators, so preventing it there aborts
with nothing to unwind. The handler looks the verb and path up in a route
table, runs the wasm work, and passes the HTML a server would have returned to
`htmx.swap()`, which does the targets, the out of band swaps and the settling
exactly as it would for a real response.

Routes are written the way server handlers are:

| Route              | Does                                                     |
| ------------------ | -------------------------------------------------------- |
| `GET /hh/status`   | The badge, once the module has initialised libsodium.    |
| `GET /hh/view/*`   | A tab's markup, out of a `<template>`.                   |
| `POST /hh/probe`   | Carrier facts and whether the payload fits. Swaps the codec field out of band, since a JPEG has no codec to choose. |
| `POST /hh/encode`  | `hh encode`.                                             |
| `POST /hh/decode`  | `hh decode`.                                             |
| `POST /hh/analyse` | `hh analyse`, over an upload or a path already in MEMFS. |
| `POST /hh/verbose` | Flips core's `verbose` flag.                             |

Core's `INFO` and `ERROR` output still goes to stdout and stderr; the page
installs Emscripten's `print` and `printErr` hooks and shows it in the log
pane, so a reader sees what the CLI would have printed.

## CI

`.github/workflows/callable-web.yml` builds the module, runs `smoke.mjs` against
it, and uploads `index.html` plus `dist/` as a Pages artifact. `ci.yml` calls it
beside the three test suites, and deploys on a push to main or once a release is
published — gated on the suites, the formatting and the coverage run all having
passed.

The Emscripten SDK is pinned and cached by version, and the libsodium build is
cached separately, keyed on `build.sh`, so a warm run skips the slowest part.

Publishing needs **Settings → Pages → Source** set to **GitHub Actions** once;
until then the deploy job fails with nothing to deploy to.

## Known limits

- **Everything runs on the main thread.** Argon2 through `crypto_pwhash` blocks
  it for around a second on an encrypted container, so the page yields a frame
  to paint its busy state before calling in. Moving the module into a worker
  would fix it properly.
- **`third_party/flag.c` is left out of the wasm build.** It asserts that
  `size_t` is as wide as `unsigned long long`, which wasm32 is not. It is the
  CLI's argument parser and nothing in a browser has an argv.
- **libsodium is built from source.** It is not an Emscripten port, unlike
  libjpeg. The first build takes a few minutes; after that it is cached in
  `.deps`.
