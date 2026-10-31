# stb_truetype

`stb_truetype.h` v1.26 from https://github.com/nothings/stb (Sean Barrett / RAD Game Tools), unmodified.
Dual licensed MIT or public domain (see the end of the header). Used only by `render/font_atlas.cpp` to bake a
signed distance field font atlas at startup.

## TODO: move to vneio

vneio's `deps/external/stb_image` module ships `stb_image`, `stb_image_write` and `stb_image_resize2`, but not
`stb_truetype`. Planned steps:

1. Add `stb_truetype.h` (and its implementation translation unit) to vneio's `stb_image` module.
2. Bump vneio inside vnerhi (`deps/internal/vnerhi/deps/internal/vneio`), then vnerhi inside vnexr.
3. Build `vne::io::image` in vnexr (vnerhi only builds vneio when its samples or mesh I/O are on).
4. Link it from `samples/framework/CMakeLists.txt` and delete this directory.
