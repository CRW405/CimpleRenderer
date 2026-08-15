# CimpleRenderer

A 3D renderer for the terminal, written from scratch in C. It loads an OBJ
mesh, projects it through a simple rotating perspective camera, and draws it
straight to your terminal — either as plain ASCII line art or as colored
"pixels" using half-block glyphs for double the vertical resolution.

## Build

```sh
mkdir -p build && cd build
cmake ..
make
```

Requires a C compiler and CMake; links against libm.

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
| `-v N`      | `--fov N`     | Field of view factor (default `1.5`)                                             |
| `-x`        | `--pixel`     | Pixel mode: pack two vertical samples per terminal row with colored half-block glyphs, doubling effective vertical resolution |
| `-n`        | `--no-fill`   | Disable triangle fill; render wireframe only                                      |
| `-c`        | `--no-cull`   | Disable backface culling; render front- and back-facing triangles alike           |

### Controls

- `W` / `A` / `S` / `D` — rotate the camera
- Mouse scroll — zoom in/out
- `Q` — quit

## OBJ support

Faces are parsed regardless of index format (`v`, `v/vt`, `v//vn`,
`v/vt/vn`) or polygon size — n-gons are fan-triangulated automatically, so
quad-heavy exports (e.g. from Blender) load correctly alongside plain
triangulated meshes.

## Sample models

`obj/` includes a few example meshes to try with `-o`: `cube.obj`,
`sumn.obj`, and the Utah teapot (`UTAH_BLEND.obj`).

## Notes

### TODO

- free fly camera
- mouse panning
- mouse rotation
- ASCII shading
- pixel shading
- glb support
