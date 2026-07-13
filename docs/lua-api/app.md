# App

*Implemented in* `src/lua/luaappapi.cpp`.

Application-level primitives — concerns that belong to the running app/game itself, not to any one engine subsystem (camera, sound, sprites, ...).

## `app_set_name(name)`

Set the application window's title. Scarab (the engine) has no opinion on what a game calls itself — the window opens with a generic default title (`"Scarab"`, `main.h`'s `APP_NAME`) before any Lua exists to override it, since the window has to exist before `main.lua`'s first line runs. Call this once, early, with the game's real name:

```lua
-- main.lua
app_set_name( "Caravellius" )
```
