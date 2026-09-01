# resources/fonts/

Shared, open-source font assets — part of `resources/` at the repo root, the general-purpose shared-resources area shipped with Scarab itself (see the root [README.md](../../README.md)), free for this repo's own samples *and* for any community game built on Scarab to use.

## caravellius8x8

`caravellius8x8.fnt`/`.png` and `caravellius8x8_bold.fnt`/`.png` — an original AngelCode BMFont bitmap font, created for (and named after) **Caravellius**, contributed by its author for shared use here. Royalty-free; no separate license file needed beyond this project's own [zlib License](../../LICENSE). A `.fnt` file references its own bitmap atlas by filename only (`page id=0 file="caravellius8x8.png"`, etc.) — keep each pair in the same directory, don't rename one without the other.

**Currently can't be loaded via `set_font`** — see "A real, verified engine bug" below.

## pressstart2p-regular.ttf

"Press Start 2P" by The Press Start 2P Project Authors (cody@zone38.net), licensed under the **SIL Open Font License 1.1** (full text: [ofl-pressstart2p.txt](ofl-pressstart2p.txt)), fetched unmodified from the canonical [google/fonts](https://github.com/google/fonts/tree/main/ofl/pressstart2p) repository. Kept under its own real filename deliberately — "Press Start 2P" is a Reserved Font Name under the OFL, and this copy is unmodified, so renaming it would misrepresent what it actually is (the opposite problem the OFL's Reserved Font Name clause exists to prevent). Used in [samples/text](../../samples/text/docs/README.md) as a real, working `set_font` demo — a single-file TrueType font, so it isn't affected by the BMFont bug below.

**Not** a stand-in specifically "based on" any other font — a from-scratch, separately-authored, openly-licensed font chosen for a similar retro pixel-computer aesthetic.

## A real, verified engine bug: multi-file BMFont loading currently fails

Confirmed live building [samples/text](../../samples/text/docs/README.md) (2026-09-01): `set_font("resources/fonts/caravellius8x8.fnt")` fails — raylib's own `LoadBMFont` reads the `.fnt` text fine, but then fails to load the atlas it references, falling back to the engine's built-in default font. The exact same files, loaded via a bare, un-hooked raylib program with no other change, load perfectly — so this isn't a bad or corrupt asset.

**Root cause, isolated precisely**: sunlight's `RaylibEngine` constructor calls raylib's `::SetLoadFileDataCallback()`, redirecting every raylib-internal binary file read (`LoadFileData()`, which `LoadImage`/texture loads go through) through `SunLight::FileSystem::ReadFile()` — the same mount-based virtual filesystem every other resource type in this engine already uses (confirmed by reading `raylibengine.cpp`'s own doc comment on this). That part works correctly, and is *not* the bug (an earlier draft of this note wrongly assumed `set_font` bypassed the mount system entirely — it doesn't; only the top-level `.fnt` *text* read, via `LoadFileText`, isn't hooked and still uses a native reader).

The actual failure is one level deeper: for a multi-file BMFont, raylib's own `LoadBMFont` (`rtext.c`) builds the atlas image's path *internally* as `TextFormat("%s/%s", GetDirectoryPath(fileName), imageFileName)` — and `GetDirectoryPath` always prepends a leading `"./"` when the input path (here, `"resources/fonts/caravellius8x8.fnt"`) doesn't itself start with `/`. The resulting `"./resources/fonts/caravellius8x8.png"` is what actually reaches the hooked `LoadFileData()` → `SunLight::FileSystem::ReadFile()` → `PHYSFS_openRead()` — and that `./`-prefixed form fails to resolve against the mounted filesystem (silently — `PhysFS`/`ReadFile` return `false`/`nullptr` with no trace of their own, unlike raylib's *default*, un-hooked reader, which does log a `FILEIO` line either way — the total absence of any `FILEIO`/`IMAGE` log line for the atlas load, confirmed live, is itself the tell). A leading `/` on the *top-level* `.fnt` path was tried as a workaround and made things *worse* — it broke resolution of the `.fnt` file itself too, so that isn't a fix either.

**Net effect**: a single-file font (TrueType/OpenType, `.ttf`/`.otf` — no second internally-referenced file, like `pressstart2p-regular.ttf` above) is unaffected, since there's no internal `GetDirectoryPath`-constructed secondary path involved at all. Only a *multi-file* BMFont atlas hits this. This is a real bug in how sunlight's raylib backend and its `SetLoadFileDataCallback` hook interact with raylib's own internal multi-file BMFont loading — living in `sunlight`, not something fixable from Scarab's own side; flag it there before attempting to actually use this font pair via `set_font`.
