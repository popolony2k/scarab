# resources/sprites/

Shared sprite-sheet assets — part of `resources/` at the repo root, the general-purpose shared-resources area shipped with Scarab itself (see the root [README.md](../../README.md)), free for this repo's own samples *and* for any community game built on Scarab to use.

`sunny_idle_down.png` — a 128×32 strip of four 32×32 frames, [sunlight](https://github.com/popolony2k/sunlight)'s own sample character from its [`sprite` sample](https://github.com/popolony2k/sunlight/tree/main/samples/sprite) — shared here unmodified since both repos are owned/authored by the same project (the same rationale as [resources/tilemap/](../tilemap/README.md)'s map).

Used in [samples/sprite](../../samples/sprite/docs/README.md) as a `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR` animated, keyboard-movable sprite.

`glow.png` (128×128, a warm radial glow whose alpha falls off from the centre; its transparent pixels keep the warm colour) and `shadow.png` (128×64, a soft bluish ellipse; its transparent pixels are **white on purpose**) — both from [sunlight](https://github.com/popolony2k/sunlight)'s own [`blend` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/blend) (commit `bfce598`), shared here unmodified for the same reason as above. The white transparent pixels in `shadow.png` are a deliberate test: a naive multiply blend paints them as a white box around the shadow, sunlight's exact one must not.

Used in [samples/blend](../../samples/blend/docs/README.md): the glow as a sprite whose blend mode the number keys choose, the shadow as a sprite that always multiplies.
