# JSON

*Implemented in* `src/lua/luajsonapi.cpp`, wrapping `nlohmann::json`.

## `load_json(path) -> table`

Read a JSON file and return its content as an equivalent Lua table — this is the engine's only config-file bridge; everything else in `resources/configs/*.json` is read this way rather than through any Caravellius-specific loader. Returns `nil` (and logs an error to stderr) if the file can't be opened or fails to parse.

JSON objects become Lua tables keyed by string; JSON arrays become Lua tables with sequential integer keys (`1`-based, matching Lua convention, via `ipairs`-friendly numbering — not JSON's own `0`-based indexing).

```lua
local data = load_json(BASE_PATH .. "configs/soundfile.json")

for _, pair in ipairs(data) do
  print(pair[1], pair[2].file_name)
end
```

```json
[
  ["ID_CARAVELLIUS_SHOOT_AUDIO", { "file_name": "/audio/global/caravellius-shot.wav" }]
]
```

Given the JSON above, `data[1][1]` is `"ID_CARAVELLIUS_SHOOT_AUDIO"` and `data[1][2].file_name` is `"/audio/global/caravellius-shot.wav"`.

`load_json` is fully generic — it has no concept of any particular file's schema, it just walks whatever structure the JSON actually contains. Nested objects/arrays convert recursively; numbers, strings, booleans, and `null` (→ Lua `nil`) convert directly.
