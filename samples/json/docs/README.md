# json sample

`load_json`, the engine's only config-file bridge. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/json/project.json
```

## What it shows

- [data.json](../data.json) — a small object with a boolean, a string, and a nested array of objects.
- [main.lua](../main.lua) — `load_json` returns `nil` (and logs to stderr) on a bad path or a parse failure, so this sample checks for that before touching the result. JSON objects become string-keyed Lua tables (`data.title`, `data.debug`); JSON arrays become `1`-based, `ipairs`-friendly tables (`data.enemies`), exactly as `load_json`'s own doc describes — confirmed live, `data.enemies[1].name` reads back as `"satellite"`.

## Lua API reference

- [`load_json`](https://popolony2k.github.io/scarab/lua-api/json.html)
