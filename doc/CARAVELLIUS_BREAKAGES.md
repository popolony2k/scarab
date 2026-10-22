# Breakages for Caravellius

What Scarab's master-plan work changes for Caravellius, the game built on Scarab. Nothing
here has been sent to Caravellius yet. It is told when this plan is released, as agreed
with the owner.

This file lives on `master_plan` only. It is removed when this plan is merged to `main`,
after Caravellius has been told what changed.

**Status:** `merged` = on `master_plan`, unreleased. `pending` = on the working branch, not
yet merged.

## Merged to master_plan

| # | Change | Who it affects | What to do at integration |
|---|--------|-----------------|---------------------------|
| 1 | **V1 (sunlight BREAKAGES 24, 46)** `camera_move_*` / `view_move_camera_*` stop exactly on the map's edges. Up/left now stop on the map's real far edge, `-ceil(view size / zoom - map size)` pixels from the origin (shown as a positive `camera_get_position()`), with a partial last step when less than a step is left. Before (sunlight v0.38.0) they could scroll up to about one tile *past* that edge, in whole steps. Down/right now come back to exactly `0`; before, they stopped one step short (with Scarab's default 1 px step: at `1`, not `0` - read from v0.38.0's code, not measured). Measured on `resources/tilemap/test.tmx` with a 7 px step: zoom 3.8125, 1240x900 view: far edge 314/403 (was 315/427), back to 0 (was 7). Zoom 1.0, 300x200 view: 340/440 (was 371/469). | Any game that scrolls with these calls. Caravellius scrolls down with `camera_move_down()` (`src/camera.lua`, per the example in Scarab's own `camera_move_up` doc - not re-checked in Caravellius' repo), so its scroll now ends at camera y `0`, not `1`. Any code that compares the camera against a fixed end value, or that works out a tile or a wave from the camera by dividing by the step, may see different numbers. | Check `src/camera.lua` and anything that reads `camera_get_position()` against a fixed end value or divides it by the step. Re-run the intro/demo timing that depends on where the scroll ends. |
| 2 | **P4 (sunlight BREAKAGES 50, "I1")** Semi-transparent drawing is no longer darker than authored. Before, every frame was drawn into a render target whose own alpha dropped under each semi-transparent draw, and that target was then blended over a black window, so anything semi-transparent came out darker (eg. `test.tmx`'s 0.65/0.69 cloud layers: a pixel at 167 173 175 instead of Tiled's 242 250 253). Opaque drawing is unchanged. | Anything semi-transparent: layers below full opacity, object and sprite opacity, soft sprite edges, text edges, and the screen fade. No code change is needed. | Recheck any colour, alpha or fade value that was tuned by eye to make up for the darkening, and any screenshot or golden-image comparison. Compare semi-transparent layers, the HUD/text and the screen fade with how they were authored. |

## Pending (on the working branch, not merged)

| # | Change | Who it affects | What to do at integration |
|---|--------|-----------------|---------------------------|
