# input sample

Keyboard and gamepad reading, before any sprite exists — see the [sprite](../../sprite/docs/README.md) sample for the same kind of movement applied to a real sprite. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/input/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move the square (`input_is_key_down`) |
| Gamepad 0 left stick (held) | Move the square (`input_get_gamepad_axis`, with a deadzone) |
| `R` | Reset the square to the center (`input_is_key_released`) |
| Gamepad 0 face-down button | Reset the square to the center (`input_is_gamepad_button_down`) |

## What it shows

- [main.lua](../main.lua) — `input_is_key_down` for continuous movement (checked every frame, true for as long as the key is held), `input_is_key_released` for an edge-triggered action (true only on the one frame the key transitions from down to up — not while holding it).
- The gamepad's left stick drives the exact same movement, not just a HUD readout — `input_get_gamepad_axis` returns a raw `-1.0`–`1.0` value that jitters near `0` even when the stick is physically centered, so a small deadzone (`__STICK_DEADZONE`) is applied before treating it as real input, the same pattern `input_is_gamepad_button_down`'s own doc example uses.
- `input_add_gamepad(0)` is called once at startup — a gamepad id has to be registered this way before any `input_is_gamepad_button_*`/`input_get_gamepad_axis` call for that id reports real state. It's harmless to keep reading gamepad 0's state every frame even with nothing plugged in — every value just reads back as centered (`0.0`) or not-pressed (`false`), never an error.

## Lua API reference

- [`input_is_key_down`/`input_is_key_released`/`input_add_gamepad`/`input_get_gamepad_axis`/`input_is_gamepad_button_down`](https://popolony2k.github.io/scarab/lua-api/input.html)
