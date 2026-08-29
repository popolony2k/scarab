# Sound

*Implemented in* `src/lua/luasoundapi.cpp` — both `sound_*` and the `sp_*_song`/`*_song` forms live in this one file now.

All sound ids are plain integers you choose (Caravellius keeps its own names for them in `caravellius/src/soundids.lua`) — the engine never interprets what a specific id means, it's just a key into `SoundManager`.

## Loading and direct playback state — `sound_*`

These are unconditional, immediate calls — no queue involved, and they don't distinguish "background music" from "sound effect." Use these for anything you want to control precisely and immediately.

### `sound_load(id, path) -> success`

Load a sound file and associate it with `id`. Must be called once before any other `sound_*`/`*_song` call for that id.

```lua
sound_load(1, BASE_PATH .. "audio/global/caravellius-shot.wav")
```

### `sound_unload(id) -> success`

Free the sound previously loaded for `id`.

### `sound_play(id) -> success`

Start playing `id` from the beginning (or resume if already loaded and stopped).

```lua
sound_play(1)
```

### `sound_stop(id) -> success`

Stop `id` if it's currently playing.

### `sound_pause(id) -> success`

Pause `id` without resetting its playback position.

### `sound_resume(id) -> success`

Resume a previously paused `id`.

### `sound_is_playing(id) -> playing`

```lua
if sound_is_playing(1) then
  print("still playing")
end
```

### `sound_set_volume(id, volume) -> success`

Set `id`'s playback volume, independently of every other loaded sound — `0.0` (silent) to `1.0` (max). Intended for crossfading between two songs (e.g. fading a stage's BGM out while a boss's BGM fades in): ramp each song's volume in opposite directions over several frames from `on_update`.

```lua
sound_set_volume(ID_DESTRUCTION_ALIENS_ATTACK_BGM, 0.5)
```

## Song commands — queued vs. direct

Both forms end up calling the same underlying playback, but only the **queued** (`sp_*`) forms — and `play_song_looping` — mark a song as "the currently tracked background music": the engine automatically re-triggers that tracked song every frame once it finishes (`EngineHost::RunScriptMachine`'s BGM-loop check), giving free looping. Plain `play_song` plays once and is never auto-repeated, which is what you want for a one-off sound effect (a shot, an explosion) rather than music.

| | Queued (participates in the `sp_*` command queue, becomes the looping BGM) | Direct, looping (immediate, still becomes the looping BGM) | Direct, one-shot (immediate, never looped) |
| --- | --- | --- | --- |
| Play | `sp_play_song(id)` | `play_song_looping(id)` | `play_song(id)` |
| Pause | `sp_pause_song(id)` | — | `pause_song(id)` |
| Stop | `sp_stop_song(id)` | — | `stop_song(id)` |
| Resume | `sp_resume_song(id)` | — | `resume_song(id)` |

```lua
-- Background music: looping, sequenced with the rest of the stage's queue
sp_play_song(ID_DESTRUCTION_ALIENS_ATTACK_BGM)

-- Background music: looping, but needs to start immediately rather than
-- wait for a possibly-stuck sp_* queue (eg. a stage script's own
-- perpetual wave-spawn loop) - a boss BGM crossfade, for example
play_song_looping(ID_BOSS_TIME_BGM)

-- A one-off sound effect: fires immediately, never looped
play_song(ID_CARAVELLIUS_SHOOT_AUDIO)
```

`play_song_looping` only exists for the Play case — pausing/stopping/resuming a tracked BGM works the same way (and un-tracks it, if applicable) whether it was started via `sp_play_song` or `play_song_looping`, so `pause_song`/`stop_song`/`resume_song` cover both.

None of the 9 song functions return a value — check `sound_is_playing(id)` if you need to know playback state.
