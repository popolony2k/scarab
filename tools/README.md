# tools/

Lua-based tooling for Scarab itself, run via `scarab.exe` the same way any game entry script would be — not part of any game's own bundle.

## pack.lua — content encryption packaging tool

See the root [README.md](../README.md#content-encryption-lock)'s own "Content encryption" section for the full design. Encrypts a game's own loose source directory (Lua scripts + every asset) into a single `.zip` bundle, using a shared secret compiled into *this specific build* of `scarab` — only that same build can decrypt what it packs. This is deterrence, not unbreakable DRM.

Two equivalent ways to run it — `--pack` (recommended) needs no `cd` into this repo at all; the direct form is the underlying mechanism `--pack` is sugar for, useful for e.g. scripting a build from inside this repo's own tree.

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

```json
{ "source_dir": "/absolute/path/to/my_game_source", "output": "/absolute/path/to/my_game.zip" }
```

Where this file itself can live depends on which invocation form you use (step 3):

- **`--pack`**: anywhere on disk — `main.cpp` reads it via a plain native file read, no location restriction at all.
- **Direct form**: at **this repo's own root** — `pack.lua` reads it via `load_json`, which (like every other resource load in this engine) goes through the mount-based virtual filesystem, so it must be reachable from wherever you run `scarab` from (the repo root, by convention — see step 3). It's gitignored (`.gitignore`), so it's safe to keep one there pointing at paths private to your own machine.

`source_dir`/`output` themselves are read/written via `pack_*`/`crypto_*` primitives, which bypass the mount-based virtual filesystem entirely and take real native OS paths regardless of invocation form — a **relative** path in either field resolves against **`pack-config.json`'s own directory** (relocatable, like a `project.json`'s `main_script` — not against whatever your shell's current directory happens to be); an **absolute** path is used unchanged, anywhere on disk, including a different project altogether having nothing to do with this repo. See [pack-config.example.json](pack-config.example.json) for a starting point.

### 3. Run the packer

**Recommended — `scarab --pack`, from any directory:**

```shell
/path/to/scarab --pack /path/to/pack-config.json
```

No window opens; the tool packs and exits immediately, printing progress to the terminal. This works from anywhere — `scarab` doesn't need to be run from its own build/repo directory, and `pack-config.json` doesn't need to live in any particular place either.

**Direct form — run from this repo's own root**, exactly like every `samples/` entry already is (a gitignored `pack-config.json` living at the repo root, read via `load_json` — see `.gitignore`):

```shell
cd /path/to/scarab-repo
./build/scarab tools/pack.lua
```

Unlike `--pack`, this form briefly opens a real window — but it closes itself automatically once packing finishes (`app_quit()`, sunlight v0.19.0+), same as `--pack`'s own process exiting on its own; nothing to close manually either way.

Either way: every file under `source_dir`, recursively, gets encrypted and added to `output` — a genuine, standard `.zip` (readable by any zip tool for its structure/filenames; only each file's *content* is opaque).

### 4. Run the packed bundle

```shell
/path/to/scarab my_game.zip
```

...once Scarab's own runtime decrypt hook exists (checkpoint 3 of the plan doc — not done yet as of this writing). Until then, a packed bundle's content can be produced but not yet read back by a running game.
