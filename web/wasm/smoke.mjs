/*
 * Drives the built module the way index.html does -- files through MEMFS, the
 * entry points through ccall -- and checks that what goes in comes back out.
 *
 *   node web/wasm/smoke.mjs
 *
 * It exists because everything else about the wasm build is only observable in
 * a browser, and a round trip that silently returns the wrong bytes is exactly
 * the failure a page will not show you.
 */
import { readFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'
import { dirname, join } from 'node:path'

const here = dirname(fileURLToPath(import.meta.url))
const root = dirname(dirname(here))

const { default: createHushHush } = await import(join(here, '..', 'dist', 'hushhush.mjs'))

const log = []
const Module = await createHushHush({ print: (t) => log.push(t), printErr: (t) => log.push(t) })

if (Module.ccall('hh_init', 'number', [], []) !== 0)
  throw new Error('hh_init failed')

const call = (name, ret, types, args) => Module.ccall(name, ret, types, args)

const readJson = (pointer) => {
  if (!pointer) return null
  const text = Module.UTF8ToString(pointer)
  call('hh_free', null, ['number'], [pointer])
  return JSON.parse(text)
}

Module.FS.mkdir('/work')

let failures = 0
const check = (label, ok, detail = '') => {
  console.log(`${ok ? 'ok  ' : 'FAIL'}  ${label}${detail ? `  ${detail}` : ''}`)
  if (!ok) failures++
}

const secret = Buffer.from('the quick brown fox jumps over the lazy dog\n'.repeat(24))

for (const [carrier, codec, passphrase] of [
  ['hush.png', 'lsbm', ''],
  ['hush.png', 'lsbr', 'correct horse battery staple'],
  ['hush.jpg', 'dct', ''],
  ['hush.jpg', 'dct', 'correct horse battery staple'],
]) {
  const label = `${carrier} / ${codec} / ${passphrase ? 'encrypted' : 'plain'}`
  log.length = 0

  Module.FS.writeFile('/work/carrier', readFileSync(join(root, 'assets', carrier)))
  Module.FS.writeFile('/work/secret', secret)

  const probe = readJson(call('hh_analyse', 'number', ['string'], ['/work/carrier']))
  check(`${label}: analyse`, probe !== null && probe.capacity > 0, probe && `${probe.type} ${probe.width}x${probe.height}, capacity ${probe.capacity}B`)

  const encoded = call('hh_encode', 'number', ['string', 'string', 'string', 'string', 'string'],
    ['/work/carrier', '/work/secret', '/work/stego', codec, passphrase])
  check(`${label}: encode`, encoded === 0, encoded !== 0 ? log.join(' | ') : '')
  if (encoded !== 0) continue

  const length = call('hh_decode', 'number', ['string', 'string', 'string'], ['/work/carrier', '/work/out', passphrase])
  check(`${label}: decode of the untouched carrier fails`, length < 0)

  const decoded = call('hh_decode', 'number', ['string', 'string', 'string'], ['/work/stego', '/work/out', passphrase])
  check(`${label}: decode`, decoded === secret.length, decoded < 0 ? log.join(' | ') : `${decoded} bytes`)
  if (decoded < 0) continue

  check(`${label}: round trip`, Buffer.from(Module.FS.readFile('/work/out')).equals(secret))

  if (passphrase) {
    const wrong = call('hh_decode', 'number', ['string', 'string', 'string'], ['/work/stego', '/work/out', 'not the passphrase'])
    check(`${label}: wrong passphrase is rejected`, wrong < 0)
  }
}

log.length = 0
Module.FS.writeFile('/work/junk', Buffer.from('not an image at all'))
check('a non-image is refused', readJson(call('hh_analyse', 'number', ['string'], ['/work/junk'])) === null)

console.log(failures ? `\n${failures} failed` : '\nall passed')
process.exit(failures ? 1 : 0)
