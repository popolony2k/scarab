# actions sample

Rebindable controls saved for the player, and gamepad rumble. A square runs and jumps, but the game never asks for a key: it reads **actions** (`"left"`, `"right"`, `"jump"`), each bound to one key, gamepad button or stick direction. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/actions/project.json
```

## Controls

| Input | Action |
| --- | --- |
| The bound keys or pad buttons (defaults: `Left` / `Right` / `Space`) | Move and jump (`input_is_action_active`) |
| `R`, then a key or a pad button | Rebind "jump" to it (`input_bind_action_key`, `input_bind_action_button`) |
| `S` | Save the bindings into the save directory (`input_save_action_map`) |
| `D` | Load the shipped defaults again (`input_load_action_map`) |
| `T` | Shake every pad that can rumble (`input_set_rumble`) |

## What it shows

- **Actions:** the movement code reads only `input_is_action_active( "jump" )` and the like, so rebinding never touches it. One input serves one action: rebinding "jump" to a key "left" already uses is refused, and the HUD shows why.
- **Saved for the player:** [project.json](../project.json) names the save folder (`"save_directory": "Scarab-actions-sample"`), which Scarab puts in the OS's per-user data folder (macOS `~/Library/Application Support/`, Windows `%APPDATA%\`, Linux `~/.local/share/`); the HUD shows the real folder (`app_get_save_directory`). At start-up the sample loads the player's `bindings.txt` from there if it exists, else the defaults it ships in [default_bindings.txt](../default_bindings.txt). Press `S`, restart, and your bindings are back.
- **Rumble:** the HUD lists each connected pad and whether it can vibrate (`input_is_rumble_supported`). A pad's jump shakes that pad for a moment; `T` shakes them all.
- Run it with `--save-dir <folder>` to keep its saves somewhere else, as the smoke tests do.

## Lua API reference

- [Input: actions and rumble](https://popolony2k.github.io/scarab/lua-api/input.html), [App: save directory](https://popolony2k.github.io/scarab/lua-api/app.html)
