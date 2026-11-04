# saves sample

A game's options and high scores, kept between runs. Each is a plain Lua table, saved as JSON into the game's save directory with `save_write` and read back with `save_read`. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/saves/project.json
```

## Controls

| Input | Action |
| --- | --- |
| `Left` / `Right` | Volume down / up (an option) |
| `F` | Show or hide the FPS counter (an option, applied at once) |
| `C` | Change the square's colour (an option) |
| `Space` | Play a round: a random score, kept if it is among the best five |
| `S` | Save the options and the scores (`save_write`) |
| `X` | Delete both saves (`save_exists`, `save_delete`) |

## What it shows

- **Two saves, two shapes:** `options.json` is a table with string keys (a JSON object); `scores.json` is a list of tables (a JSON array). `save_read` gives back the same tables, numbers as integers.
- **Defaults when there is no save:** at start-up each save is read with `save_read`, and the sample uses its own defaults when the answer is `nil, "no save named ..."` (the HUD shows that message on a first run).
- **Kept between runs:** press `S`, quit, launch again, and your options and scores are back. `X` deletes them, so the next launch starts from the defaults.
- **Where saves go:** [project.json](../project.json) names the save folder (`"save_directory": "Scarab-saves-sample"`), which Scarab puts in the OS's per-user data folder (macOS `~/Library/Application Support/`, Windows `%APPDATA%\`, Linux `~/.local/share/`); the HUD shows the real folder. The files are plain, readable JSON.
- Run it with `--save-dir <folder>` to keep its saves somewhere else, as the smoke tests do.

## Lua API reference

- [Save](https://popolony2k.github.io/scarab/lua-api/save.html), [App: save directory](https://popolony2k.github.io/scarab/lua-api/app.html)
