# Timers

*Implemented in* `src/lua/luaengine.cpp` (`LuaEngine::SetTimer`/`ResetTimer`), backed by `SunLight::Concurrent::Timer`.

Unlike `sp_wait` (which only pauses the `ScriptProcessor` queue), a timer fires its callback repeatedly on its own schedule, independent of the queue, from a **background thread** — see the important thread-safety note below.

## `set_timer(id, intervalMilli, callback)`

Register a new periodic timer. `id` is any integer you choose, used later to cancel it via `reset_timer`. `callback` is a Lua function taking no arguments, called every `intervalMilli` milliseconds until `reset_timer(id)` is called.

Registering a `set_timer` with an `id` that's already active is a no-op (logs an error) — always `reset_timer` before re-registering the same id.

```lua
local shotsFired = 0

set_timer(1, 1000, function()
  shotsFired = shotsFired + 1
  print("ticks: " .. shotsFired)
end)
```

## `reset_timer(id)`

Stop and remove a timer previously registered with `set_timer`. Safe to call even if you're not sure the timer is still running — logs an error (not a crash) if `id` doesn't exist.

```lua
reset_timer(1)
```

## Thread safety — read this before using timers

A timer's callback runs on its **own background thread**, not the main game thread. The engine already serializes every one of its own Lua entry points (per-frame update, wave dispatch, etc.) against timer callbacks internally, so calling ordinary engine primitives (`sound_play`, `sprite_get_pos`, ...) from inside a timer callback is safe.

What *isn't* automatically safe: **don't call `reset_timer` on a timer's own id from inside that same timer's callback** — behavior in that specific case isn't gracefully handled and this pattern hasn't been exercised. If you need a one-shot timer, prefer a plain counter guard inside the callback (do the work only once, then leave the timer running idle, or cancel it from the main-thread `on_update` instead of from within the callback itself):

```lua
local fired = false

set_timer(2, 500, function()
  if fired then return end
  fired = true
  print("fires exactly once")
end)
```
