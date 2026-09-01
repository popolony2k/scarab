# sound sample

Direct sound effects and all three ways to play a "song," all against the same loaded audio. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/sound/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `1` | `play_song` — direct, one-shot, never auto-repeats |
| `2` | `play_song_looping` — direct, but *tracked*: the engine auto-replays it once it ends |
| `3` | `sp_play_song` — queued (participates in the `sp_*` command queue), also tracked |
| `P` | `pause_song` — covers both tracked forms (`2`/`3`) identically |
| `O` | `stop_song` |
| `R` | `resume_song` |
| `Up` / `Down` | Adjust volume (`sound_set_volume`) — tracked in a local variable since there's no `sound_get_volume` to read it back |

## What it shows

- [main.lua](../main.lua) — `sound_load` once at startup, then every Play form from the [song-commands comparison table](https://popolony2k.github.io/scarab/lua-api/sound.html): only the *tracked* forms (`play_song_looping`, `sp_play_song`) get free auto-looping from the engine once playback ends — plain `play_song` plays once and is never auto-repeated, which is what a one-off sound effect (a shot, an explosion) actually wants.
- `pause_song`/`stop_song`/`resume_song` work identically regardless of which tracked form started playback.
- `sound_is_playing` is read every frame to show real playback state on the HUD, since none of the 9 song functions themselves return a value.

## Lua API reference

- [`sound_load`/`sound_is_playing`/`sound_set_volume`/`play_song`/`play_song_looping`/`sp_play_song`/`pause_song`/`stop_song`/`resume_song`](https://popolony2k.github.io/scarab/lua-api/sound.html)
