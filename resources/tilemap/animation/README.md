# resources/tilemap/animation/

`animation.tmx` is Scarab's own map for Tiled **tile animation**, made for master-plan item V13 (sunlight I11, BREAKAGES 44 and 60). It is a 40x12 map of 32 px tiles, each case labelled with a text object (font family `"Sans"`, so it needs `register_font`). Its tileset `tileset_anim.tsx` uses Scarab's monkey icons one folder up (`../images/monke_variants.png`).

- **Tile 0 (gid 1)** animates over three icons (gids 1, 6, 11) for **100, 50 and 300 ms**, a 450 ms cycle. 50 ms is not a whole number of 60 fps frames, so a frame held one draw too long, or a cycle that stretches, shows up.
- Row 2 holds it in several cells: two plain ones, a horizontally flipped one, and one far to the right (column 36, off screen at first). Next to them are a **static** tile (gid 16) and a tile whose animation durations are **all 0 ms** (gid 4: it shows its first frame).
- Row 5 holds it on a **hidden layer**.
- The object layer holds it as a **tile object** (`anim_object`), flipped (`anim_object_flipped`), and a rectangle (`not_a_tile`).

The animation runs by time, as Tiled shows it: the frame shown is the time since `tilemap_load_map`, modulo the cycle. So every copy is in step, and none pauses while hidden or off screen. [samples/shapes](../../../samples/shapes/docs/README.md) shows it at real speed (key `6`). `scripts/tile_animation_smoke` checks every copy against the clock, exactly, every frame for 11 cycles.
