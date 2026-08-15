# CimpleRenderer

A 3D renderer for the terminal, written from scratch in C — no graphics
libraries, no GPU. It parses an OBJ mesh by hand, projects it through a
hand-rolled rotating perspective camera, rasterizes and shades every
triangle itself, and writes the result straight to stdout as either plain
ASCII line art or colored "pixels" using half-block glyphs.

## Build

```sh
mkdir -p build && cd build
cmake ..
make
```

Requires a C compiler and CMake; links against libm (for `sin`/`cos`/`sqrt`).

## Usage

```sh
./CimpleRenderer [options]
```

Run from `build/` so the default `-o ../obj/cube.obj` path resolves, or pass
your own `-o` path.

### Options

| Flag        | Long form     | Description                                                                     |
|-------------|---------------|----------------------------------------------------------------------------------|
| `-w N`      | `--width N`   | Render width in terminal columns (defaults to the detected terminal width)       |
| `-h N`      | `--height N`  | Render height in terminal lines (defaults to the detected terminal height)       |
| `-f N`      | `--fps N`     | Target frames per second (default `60`)                                          |
| `-o PATH`   | `--obj PATH`  | OBJ file to load (default `../obj/cube.obj`)                                      |
| `-s N`      | `--scale N`   | Object scale factor (default `1.0`)                                              |
| `-v N`      | `--fov N`     | Field of view / zoom factor (default `1.5`)                                       |
| `-x`        | `--pixel`     | Pixel mode: pack two vertical samples per terminal row with colored half-block glyphs, doubling effective vertical resolution |
| `-n`        | `--no-fill`   | Disable triangle fill; render wireframe only                                      |
| `-c`        | `--no-cull`   | Disable backface culling; render front- and back-facing triangles alike           |

### Controls

- `W` / `A` / `S` / `D` — rotate the camera (pitch / yaw)
- Mouse scroll — zoom in/out (dolly)
- `Q` — quit

## OBJ support

Faces are parsed regardless of index format (`v`, `v/vt`, `v//vn`,
`v/vt/vn`) or polygon size — n-gons are fan-triangulated automatically, so
quad-heavy exports (e.g. from Blender) load correctly alongside plain
triangulated meshes. Meshes that ship with no `vn` normals at all (common
for some scanned/converted meshes) get per-face normals computed on load
instead of silently losing shading and backface culling — see
[Normals](#4-normals-parsed-or-computed) below.

## Sample models

`obj/` includes a few example meshes to try with `-o`: `cube.obj`,
`sumn.obj`, the Stanford `bunny.obj` and `cow.obj`, `buddha.obj`, and the
Utah teapot (`UTAH_BLEND.obj`). The bunny, cow, and buddha ship with no
`vn` normals, so they exercise the dynamic-normal path described below.

## How it works

Every frame runs the same pipeline, once per triangle: **load → transform →
cull → project → shade → rasterize → composite to the terminal**. There's
no scene graph, no matrix stack, no GPU — every step below is a few lines
of plain C in `obj.c`, `draw.c`, `render.c`, and `main.c`.

### 1. Loading the mesh (`obj.c`)

`parse_obj` reads the file twice. The first pass just counts `v`, `vn`, and
`f` lines so exact-sized arrays can be allocated up front (no
`realloc`-as-you-go). The second pass fills them in.

**Fan triangulation.** An `f` line can reference any number of vertices
(triangles, quads, n-gons). For a face with vertices `v0 v1 v2 ... vk`,
the parser emits a triangle fan pivoting on the first vertex:

```
(v0, v1, v2), (v0, v2, v3), ..., (v0, v_{k-1}, vk)
```

That's `k - 1` triangles for `k + 1` vertices — e.g. a quad (4 vertices)
becomes 2 triangles. This is why the first counting pass computes
`face_count += vertex_count - 2` per polygon: that's exactly how many
triangles fan-triangulation will produce for it.

**Index formats.** Each face-vertex token can be `v`, `v/vt`, `v//vn`, or
`v/vt/vn`. `parse_face_vertex_token` tries each `sscanf` pattern in turn and
extracts the vertex index (always) and normal index (if present) — texture
coordinates are parsed but discarded, since this renderer has no texturing.

### 2. Centering the mesh (`center_mesh`)

Many OBJ exports aren't centered on their own origin. `center_mesh` finds
the axis-aligned bounding box of all vertices:

```
center = (min + max) / 2   (computed per axis)
```

and subtracts `center` from every vertex. This is done once at load time,
not per frame, so the camera can always assume the model is roughly
centered at the origin.

### 3. Camera transform & projection (`draw.c: project`)

For every vertex, per frame, `project()` does a yaw rotation, a pitch
rotation, then a perspective divide. Given a vertex `(x, y, z)` and the
global `scale` factor:

**Yaw** — rotate around the Y axis by `angle_y` (driven by `A`/`D`), acting
on the `(x, z)` plane:

```
x1 = x·scale·cos(angle_y) − z·scale·sin(angle_y)
z1 = x·scale·sin(angle_y) + z·scale·cos(angle_y)
y1 = y·scale
```

**Pitch** — rotate around the X axis by `angle_x` (driven by `W`/`S`),
acting on the `(y, z)` plane:

```
y2 = y1·cos(angle_x) − z1·sin(angle_x)
z2 = y1·sin(angle_x) + z1·cos(angle_x)
```

Both are the standard 2×2 rotation matrix `[[cosθ, −sinθ], [sinθ, cosθ]]`
applied to a coordinate pair — just written out by hand instead of through
a matrix type.

**Camera offset.** `y2 -= 0.5` nudges the view down slightly (an
eyeline-height fudge so models sit centered in frame rather than dead-center
on their vertical midpoint). `z2 += distance` then pushes the rotated point
`distance` units away from the camera along Z — this is the "dolly"
distance controlled by mouse scroll (`ZOOM_SPEED = 0.5` per tick, clamped to
`[MIN_DISTANCE, MAX_DISTANCE] = [1, 50]`). Moving the point away from a
fixed camera at the origin is mathematically identical to moving the camera
back from a fixed point, and is simpler to write.

**Perspective divide.** This is the one step that makes it look 3D instead
of orthographic — points further away (`z2` larger) end up closer to the
center of the screen:

```
x_proj = (x1 / z2) · fov
y_proj = (y2 / z2) · fov
```

`fov` (default `1.5`) plays the role of focal length in this projection: a
larger value narrows the effective field of view (zooms in), a smaller
value widens it (zooms out).

**Screen mapping.** `x_proj`/`y_proj` are in roughly `[-1, 1]`. The last
step maps that into pixel coordinates:

```
p.x = (x_proj + 1) · 0.5 · width
p.y = (1 − y_proj) · 0.5 · height
p.z = z2                              // kept as camera-space depth, for z-buffering
```

`y` is flipped (`1 − y_proj` instead of `y_proj + 1`) because camera-space Y
grows "up" while screen-space Y grows downward, row by row.

### 4. Normals (parsed or computed)

Each `Face` carries a `normal` pointer. If the file has `vn` normals,
`parse_face_line` resolves it from the first face-vertex token's `vn`
index, falling back to the mesh's first parsed normal if that particular
token didn't specify one.

If the file has **no** `vn` lines at all, every face would otherwise get a
`NULL` normal, which silently breaks both backface culling and shading (see
below — both no-op when `normal` is `NULL`). Instead, `compute_dynamic_normals`
runs once at load time and gives every face a real geometric normal, via
the cross product of two of its edges:

```
e1 = vertex2 − vertex1
e2 = vertex3 − vertex1
n  = e1 × e2 = (e1.y·e2.z − e1.z·e2.y,  e1.z·e2.x − e1.x·e2.z,  e1.x·e2.y − e1.y·e2.x)
n  = n / |n|                            // normalize to unit length
```

This is a flat per-face normal (one normal per triangle, not smoothed
across shared vertices), which is enough to restore correct culling and
give the mesh real shaded contours instead of a flat, always-lit blob.

### 5. Backface culling (`is_backface`)

A triangle faces away from the camera if its normal, once rotated into
camera space by the *same* yaw/pitch as the vertices, points into the
screen rather than out of it:

```
z1 = nx·sin(angle_y) + nz·cos(angle_y)
y1 = ny
z2 = y1·sin(angle_x) + z1·cos(angle_x)

is_backface := z2 ≥ 0
```

The camera looks down `+Z`, so a normal with a non-negative camera-space Z
component points away from (or perpendicular to) the camera — that face is
culled. This runs before projection, since a backface never needs to be
rasterized at all. `--no-cull` skips this check entirely, drawing both
sides of every triangle (needed for non-manifold meshes with
inconsistent/missing winding).

### 6. Shading (`get_shade_intensity`)

A single fixed directional light shines from `(0, −1, −1)` (down and toward
the camera). The face normal is rotated into camera space the same way as
in culling, then dotted with the light direction (a Lambertian diffuse
term):

```
light_dot = x1·0 + y2·(−1) + z2·(−1) = −(y2 + z2)
intensity = clamp(light_dot, 0, 1)
```

`intensity` is a single number in `[0, 1]`: `0` = the face receives no
light, `1` = fully lit. Every face gets one flat intensity (flat shading —
no per-vertex/per-pixel normal interpolation), computed once per face per
frame, not per pixel.

That intensity then feeds two different outputs depending on render mode:

- **ASCII mode:** bucketed into an index into the brightness ramp
  `".,-~:;=!*#$@"` (12 characters, dimmest to densest):
  `index = floor(intensity · 11)`, and `symbol_ramp[index]` is the
  character drawn for that face.
- **Pixel mode:** scaled to a byte, `shade = round(intensity · 255)`, and
  carried through the rasterizer in a `shade_buffer` parallel to the
  ASCII `pixel_buffer`, so it can be turned into an actual color at output
  time instead of a character (see [§8](#8-compositing-the-frame-renderc)).

### 7. Rasterization (`draw.c`)

Two modes, selected by `--no-fill`:

**Wireframe (`-n`).** Each triangle's 3 edges are drawn with a textbook
integer Bresenham line algorithm (`draw_line`): walk from `(x0,y0)` to
`(x1,y1)`, accumulating an error term that decides whether to step in `x`,
`y`, or both, so the line is drawn with only integer arithmetic and no
per-pixel division.

**Filled (default).** `fill_triangle` uses an edge-function (barycentric)
rasterizer instead of a naive scanline fill, specifically so that two
triangles sharing an edge (extremely common — e.g. every quad face is 2
triangles sharing a diagonal) resolve that shared edge identically on both
sides, with no double-drawn or missing pixels. For a triangle
`(A, B, C)` and a pixel `P`, define:

```
edge(A, B, P) = (B.x−A.x)·(P.y−A.y) − (B.y−A.y)·(P.x−A.x)
```

This is twice the signed area of triangle `A,B,P` — computed with plain
integer math (`long`), so it's exact, with no floating-point rounding to
disagree between neighboring triangles. `area = edge(A, B, C)` is twice the
triangle's own signed area:

- If `area == 0`, the triangle is degenerate (zero screen-space area) and is
  skipped.
- If `area < 0`, the triangle's winding is clockwise in screen space; `B`
  and `C` are swapped so `area` (and everything below) is consistently
  positive.

For every pixel in the triangle's bounding box, three edge weights are
computed — each relative to the edge *opposite* one vertex:

```
w0 = edge(B, C, P)     // weight for vertex A
w1 = edge(C, A, P)     // weight for vertex B
w2 = edge(A, B, P)     // weight for vertex C
```

`P` is inside the triangle exactly when `w0, w1, w2` are all `≥ 0`. Dividing
each by `area` gives the barycentric weights (`b0 + b1 + b2 = 1`), which
interpolate any per-vertex quantity across the triangle — here, depth:

```
z = b0·A.z + b1·B.z + b2·C.z
```

**The top-left fill rule.** Using a plain `≥ 0` test on all three edges
would double-draw pixels that sit exactly on a shared edge (both
neighboring triangles claim `w == 0` as "inside"). `is_top_left_edge`
classifies each edge's direction:

```
is_top_left(dx, dy) := (dy < 0) or (dy == 0 and dx > 0)
```

For any edge, exactly one of its two directions (`A→B` or `B→A`) satisfies
this. The triangle whose edge direction *is* top-left gets an inclusive
test (`w ≥ 0`); the other gets an exclusive one, implemented as a `-1` bias
before the same `≥ 0` check (`w − 1 ≥ 0` ⟺ `w ≥ 1`, which excludes the
`w == 0` boundary). So a boundary pixel is always claimed by exactly one of
the two triangles sharing that edge — never both (which would flicker
between them, indistinguishable from z-fighting) and never neither (a 1px
gap).

**Depth test.** `depth_buffer` is reset to `+∞` for every pixel at the start
of each frame (`render_scene` in `main.c`). Every candidate pixel from every
triangle competes on `current_z < depth_buffer[idx]`: only if this
triangle's interpolated depth is strictly closer than whatever's already
there does it win, overwriting `pixel_buffer` (the ASCII symbol),
`shade_buffer` (the light intensity byte), and `depth_buffer` itself. Depth
is carried as full-precision `double` through interpolation and only
narrowed to `float` when stored, so triangles that are very close in depth
(e.g. adjacent faces of a curved surface) still resolve correctly instead
of the coarser precision letting ties flip per-frame.

Because every triangle is depth-tested independently against the same
buffer, faces don't need to be sorted by depth (no painter's algorithm) —
draw order doesn't matter, only the depth comparison does.

### 8. Compositing the frame (`render.c`)

Once every triangle for the frame has been culled, projected, shaded, and
rasterized into `pixel_buffer`/`shade_buffer`/`depth_buffer`, `render()`
turns those buffers into actual terminal output. Two very different paths,
selected by `--pixel`:

**ASCII mode** just copies `pixel_buffer` row by row straight into the
output buffer, one byte (character) per cell, with a `\n` at the end of
each row — the character written by the rasterizer *is* the character
printed.

**Pixel mode** exploits the "▄" (lower half-block) glyph to pack two
vertical samples into one printed row, doubling effective vertical
resolution: for output row `y`, it reads `pixel_buffer` rows `2y` and
`2y+1`, painting row `2y` as the glyph's *background* color and row `2y+1`
as its *foreground* color (that's literally what the glyph looks like — a
solid block occupying the bottom half of the character cell, so the top
half shows through as the background color and the bottom half is drawn in
the foreground color). This is also why `--pixel` internally doubles
`screen_height` before allocating buffers (`main.c`: `set_height *= 2`) —
twice as many logical rows are rasterized so they can be paired up here.

Each sample's color comes from `shade_buffer` via `shade_to_rgb`: an
unlit-but-present sample maps to a dim gray floor rather than black, so it
stays visually distinct from empty background (which is forced to pure
black), and scales up linearly to white as intensity increases:

```
v = SHADE_FLOOR + (255 − SHADE_FLOOR) · (shade / 255)     // SHADE_FLOOR = 40
color = (v, v, v)                                          // grayscale
```

Colors are emitted as 24-bit "truecolor" ANSI escapes
(`\033[48;2;r;g;bm` for background, `\033[38;2;r;g;bm` for foreground).
Consecutive cells with the same color reuse the previous escape sequence
instead of re-emitting it, which keeps output size — and the bytes actually
sent to the terminal each frame — well below the worst case of one escape
per cell.

Both modes finish with the same status line (FPS, resolution, mouse
position) appended after the rendered rows, and the whole frame buffer is
written to `stdout` in a single `write()` call.

### 9. The main loop (`main.c`)

Each iteration: apply pending `WASD`/scroll input to `angle_x`, `angle_y`,
`distance` → reset `pixel_buffer`/`depth_buffer` → run steps 3–7 over every
face → composite (step 8) → read new input → sleep for whatever's left of
the target frame budget (`1,000,000 / target_fps` microseconds), measured
against `CLOCK_MONOTONIC`, to cap the frame rate. Actual FPS is derived from
total frame time (work + sleep) and shown in the status line.

## Notes

### TODO

- free fly camera
- mouse panning
- mouse rotation
- glb support
