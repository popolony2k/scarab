# Input

*Implemented in* `src/lua/luainputapi.cpp`.

All input functions poll current state once per call — there's no event/callback model for input (unlike collisions, which do have a callback — see [collision.md](collision.md)). Call these from your own per-frame update function.

## Keyboard

```lua
input_is_key_down(key) -> down       -- true every frame the key is held
input_is_key_up(key) -> up           -- true every frame the key is NOT held
input_is_key_released(key) -> released -- true only on the frame the key transitions from down to up
```

```lua
function Player.update(dt)
  if input_is_key_down(KEY_LEFT) then
    -- move left
  end
end
```

## Gamepad

```lua
input_is_gamepad_button_down(gamepadId, button) -> down
input_is_gamepad_button_up(gamepadId, button) -> up
input_get_gamepad_axis(gamepadId, axis) -> value   -- -1.0 .. 1.0
input_add_gamepad(gamepadId)                        -- register a gamepad id so it's actually polled
```

A gamepad must be registered with `input_add_gamepad` once (typically at startup) before any `input_is_gamepad_button_*`/`input_get_gamepad_axis` call for that id will report real state.

```lua
input_add_gamepad(0)

function Player.update(dt)
  if input_is_gamepad_button_down(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) then
    -- move left
  end

  -- analog stick, with a deadzone - raw axis values jitter near 0 even
  -- when the stick is physically centered
  local axisX = input_get_gamepad_axis(0, GAMEPAD_AXIS_LEFT_X)
  if axisX < -0.1 then
    -- move left
  end
end
```

## Constants

Every constant below is registered as a plain Lua global (an integer) — no `require`, no table lookup, just reference the name directly. They match raylib's own `KeyboardKey`/`GamepadButton`/`GamepadAxis`/`ControllerType` enums 1:1 (full lists in `src/lua/luainputapi.cpp` if you need one not shown here).

**Keyboard** (`KEY_*`) — the full alphabet `KEY_A`..`KEY_Z`, digits `KEY_ZERO`..`KEY_NINE`, function keys `KEY_F1`..`KEY_F12`, arrows `KEY_UP`/`KEY_DOWN`/`KEY_LEFT`/`KEY_RIGHT`, and common named keys: `KEY_SPACE`, `KEY_ENTER`, `KEY_ESCAPE`, `KEY_TAB`, `KEY_BACKSPACE`, `KEY_LEFT_SHIFT`/`KEY_RIGHT_SHIFT`, `KEY_LEFT_CONTROL`/`KEY_RIGHT_CONTROL`.

**Gamepad buttons** (`GAMEPAD_BUTTON_*`): `GAMEPAD_BUTTON_LEFT_FACE_UP`/`_DOWN`/`_LEFT`/`_RIGHT` (D-pad), `GAMEPAD_BUTTON_RIGHT_FACE_UP`/`_DOWN`/`_LEFT`/`_RIGHT` (face buttons), `GAMEPAD_BUTTON_LEFT_TRIGGER_1`/`_2`, `GAMEPAD_BUTTON_RIGHT_TRIGGER_1`/`_2`, `GAMEPAD_BUTTON_LEFT_THUMB`/`GAMEPAD_BUTTON_RIGHT_THUMB`, `GAMEPAD_BUTTON_MIDDLE_LEFT`/`_MIDDLE`/`_MIDDLE_RIGHT`.

**Gamepad axes** (`GAMEPAD_AXIS_*`): `GAMEPAD_AXIS_LEFT_X`/`_LEFT_Y`, `GAMEPAD_AXIS_RIGHT_X`/`_RIGHT_Y`, `GAMEPAD_AXIS_LEFT_TRIGGER`/`_RIGHT_TRIGGER`.

**Controller types** (`CONTROLLER_*`): `CONTROLLER_KEYBOARD`, `CONTROLLER_GAMEPAD`, `CONTROLLER_MOUSE`, `CONTROLLER_TOUCH`, `CONTROLLER_NULL`.
