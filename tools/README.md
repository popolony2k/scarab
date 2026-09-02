# tools/

Lua-based tooling for Scarab itself, run via `scarab.exe` the same way any game entry script would be — not part of any game's own bundle.

## pack.lua — content encryption packaging tool

See [docs/content-encryption-plan.md](../docs/content-encryption-plan.md) for the full design. Encrypts a game's own loose source directory (Lua scripts + every asset) into a single `.zip` bundle, using a shared secret compiled into *this specific build* of `scarab` — only that same build can decrypt what it packs. This is deterrence, not unbreakable DRM (see the plan doc's own "Motivation" section).

### 1. Build `scarab` with your own key

```shell
cmake -B build -S . -DSCARAB_CONTENT_KEY=<64 hex characters>
cmake --build build -j 4
```

A 64-character hex string decodes to the 32 raw bytes `crypto_secretbox` needs. Generate one with, e.g.:

```shell
python3 -c "import secrets; print(secrets.token_hex(32))"
```

**Never commit a real key to version control.** Keep it in a local, gitignored file or a CI secret, not typed into any tracked file.

### 2. Write a `pack-config.json`

At **this repo's own root** — `pack.lua` reads it via `load_json`, which (like every other resource load in this engine) goes through the mount-based virtual filesystem, so it must live somewhere reachable from wherever you run `scarab` from (see step 3). It's gitignored, so it's safe to point at paths private to your own machine (see [pack-config.example.json](pack-config.example.json)):

```json
{ "source_dir": "/absolute/path/to/my_game_source", "output": "/absolute/path/to/my_game.zip" }
```

`source_dir`/`output` themselves are read/written via `pack_*`/`crypto_*` primitives, which — unlike `pack-config.json`'s own location — bypass the mount-based filesystem entirely and take real native OS paths, so they can point anywhere on disk, including a different project altogether having nothing to do with this repo.

### 3. Run the packer

From **this repo's own root**, exactly like every `samples/` entry already is:

```shell
cd /path/to/scarab-repo
./build/scarab tools/pack.lua
```

Every file under `source_dir`, recursively, gets encrypted and added to `output` — a genuine, standard `.zip` (readable by any zip tool for its structure/filenames; only each file's *content* is opaque). Packing finishes almost instantly; the tool prints `Done` and, like every other Scarab entry script today, keeps the window open until you close it or press Esc (there's no programmatic "quit" primitive yet).

### 4. Run the packed bundle

```shell
/path/to/scarab my_game.zip
```

...once Scarab's own runtime decrypt hook exists (checkpoint 3 of the plan doc — not done yet as of this writing). Until then, a packed bundle's content can be produced but not yet read back by a running game.
