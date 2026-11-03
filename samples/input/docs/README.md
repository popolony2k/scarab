# input sample

Keyboard and gamepad reading for two players, before any sprite exists — see the [sprite](../../sprite/docs/README.md) sample for the same kind of movement applied to a real sprite. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/input/project.json
```

## Controls

| Input | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move player 1's square, the blue one (`input_is_key_down`) |
| A on a pad (`GAMEPAD_BUTTON_RIGHT_FACE_DOWN`) | Join: the pad claims the next free player slot (`input_claim_player_slot`) |
| B on a pad (`GAMEPAD_BUTTON_RIGHT_FACE_RIGHT`) | Leave: free the pad's slot (`input_release_player_slot`) |
| A joined pad's left stick / D-pad (held) | Move that player's square (`input_get_gamepad_axis`, `input_is_gamepad_button_down`) |
| `[` / `]` | Lower / raise the stick dead zone of every pad (`input_set_axis_deadzone`) |
| `R` | Reset both squares (`input_is_key_released`) |

## What it shows

- **Connected pads and hot-plug:** the HUD lists every pad plugged in now (`input_get_connected_gamepads`, with each pad's name) and the last connect or disconnect (`input_set_gamepad_connection_handler`). The handler is set in the entry script, so its first call reports the pads already plugged in at start-up.
- **Player slots:** the first two pads to press A become players 1 and 2; the keyboard always drives player 1 as well. Unplugging a pad keeps its slot — the HUD shows `(unplugged)`, and plugging it back in makes it that player again. B frees the slot.
- **Dead zones:** a worn stick at rest reads a little off centre. `input_get_gamepad_axis` applies the pad's dead zone (default `0.1`), so the square doesn't creep and the movement code needs no threshold of its own; `input_get_gamepad_axis_raw` returns the value as the pad reports it. The bottom HUD line shows both for player 1's pad, and `[` / `]` move the dead zone so you can watch a resting stick go from `0.00` to its raw value.
- The D-pad drives the same movement through plain `input_is_gamepad_button_down` checks — a digital input, no dead zone. **Naming gotcha**: raylib's own gamepad button naming (which this engine's `GAMEPAD_BUTTON_*` constants mirror exactly) calls the D-pad `LEFT_FACE` (`GAMEPAD_BUTTON_LEFT_FACE_UP`/`DOWN`/`LEFT`/`RIGHT`) — the face buttons (A/B/X/Y or equivalent) are `RIGHT_FACE`.
- No pad needs registering before it is read. With nothing plugged in, the pad list is empty and every read is centred or not pressed, never an error.

## Lua API reference

- [Input: keyboard, gamepad, connected gamepads, stick dead zones, player slots](https://popolony2k.github.io/scarab/lua-api/input.html)
