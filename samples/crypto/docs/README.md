# crypto sample

`crypto_encrypt_data`/`crypto_decrypt_data`, the content-encryption primitives behind [tools/pack.lua](../../../tools/README.md) (see the root [README.md](../../../README.md#content-encryption-lock)'s own "Content encryption" section for the full design). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/crypto/project.json
```

This sample's own on-screen output depends on how `scarab` was built:

- **Default build** (no `SCARAB_CONTENT_KEY` configured): `crypto_encrypt_data` returns `nil`, and the sample shows that plainly — this is a normal, expected state (not every build needs content encryption), not an error.
- **Built with a real key**:

  ```shell
  cmake -B build -S . -DSCARAB_CONTENT_KEY=<64 hex characters>
  cmake --build build -j 4
  ```

  A genuine encrypt → decrypt roundtrip runs, plus a deliberate tamper test.

Run it once against each build to see both code paths — both were verified live building this sample.

## What it shows

- [main.lua](../main.lua) — encrypts a plaintext string, decrypts it back, and confirms it matches; then flips one ciphertext byte and confirms `crypto_decrypt_data` refuses it (returns `nil`) rather than returning silently-wrong data — `crypto_secretbox`'s authenticated-encryption guarantee, confirmed live (`LuaCryptoApi: crypto_decrypt_data failed - wrong key or corrupted data.` on stderr for the tamper case only).
- Neither primitive ever takes a key argument — both are keyed entirely by the compile-time `SCARAB_CONTENT_KEY`, never a Lua-supplied value.

## Lua API reference

- [`crypto_encrypt_data`/`crypto_decrypt_data`](https://popolony2k.github.io/scarab/lua-api/crypto.html)

## See also

- [tools/README.md](../../../tools/README.md) — `tools/pack.lua`, the real, full packaging tool built on these same primitives (plus `LuaPackApi`'s `pack_*` ones) — the "Pack" Lua API category's own worked example, run via `scarab --pack <config.json>` rather than kept as a `samples/pack` — packaging is a dev-time tool operating on real native OS paths, not something that fits this repo's usual "opens a window, draws something" sample shape.
