# VneXR Scripts

Build and utility scripts for VneXR (adapted from [vnegfx](https://github.com/vertexnova/vnegfx)).

## Submodule init

```bash
chmod +x scripts/*.sh
./scripts/init_submodules.sh
```

## Build Scripts

### Linux (`build_linux.sh`)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build --dev
./scripts/build_linux.sh -c clang -j 20 -t Release --with-examples
./scripts/build_linux.sh -interactive
./scripts/build_linux.sh -clean -t Debug
```

**Options:** `-t` build type, `-c` compiler (gcc|clang), `-a` action (configure|build|configure_and_build|test), `-l` lib type, `-j` jobs, `-clean`, `-interactive`, `--dev`, `--with-tests`, `--with-examples`, `--no-tests`, `--no-examples`, `-h`

### macOS (`build_macos.sh`)

```bash
./scripts/build_macos.sh -xcode-only -t Debug -clean
open build/shared/Debug/xcode-macos-clang-*/vnexr.xcodeproj

./scripts/build_macos.sh -t Debug -a configure_and_build --dev
./scripts/build_macos.sh -t Release --library-only
```

**Options:** `-t`, `-a`, `-l`, `-xcode`, `-xcode-only`, `-j`, `-clean`, `-interactive`, `--dev`, `--with-tests`, `--with-examples`, `--no-tests`, `--no-examples`, `--library-only`, `-h`

### WebAssembly (`build_wasm.sh`)

Requires [Emscripten](https://emscripten.org/) (`source /path/to/emsdk/emsdk_env.sh`).

```bash
./scripts/build_wasm.sh -t Release -a configure_and_build
./scripts/build_wasm.sh -t Debug --no-examples
./scripts/build_wasm.sh --serve
```

**Options:** `-t`, `-a` (configure|build|configure_and_build|test|serve), `-l`, `-j`, `-clean`, `--with-examples|--no-examples`, `--with-tests|--no-tests`, `--serve`, `--kill`, `-h`

### iOS (`build_ios.sh`)

```bash
./scripts/build_ios.sh -xcode-only -t Debug -clean
./scripts/build_ios.sh -t Debug -a configure_and_build --dev -simulator
./scripts/build_ios.sh -t Release -l static -device -j 8
```

**Options:** `-t`, `-a`, `-l`, `-simulator|-device`, `-deployment-target`, `--dev`, `--with-tests`, `--with-examples`, `--library-only`, `-xcode`, `-xcode-only`, `-j`, `-clean`, `-h`

### visionOS (`build_visionos.sh`)

```bash
./scripts/build_visionos.sh -xcode-only -t Debug -clean
./scripts/build_visionos.sh -t Debug -simulator --dev
```

### Android (`build_android.sh`)

Set `ANDROID_HOME` and `ANDROID_NDK_ROOT` first.

```bash
./scripts/build_android.sh -t Debug -a configure_and_build --dev -abi x86_64
./scripts/build_android.sh -t Release -l static -abi arm64-v8a
```

**Options:** `-t`, `-a` (configure|build|configure_and_build), `-abi`, `-api`, `-l`, `-clean`, `-j`, `--dev`, `--with-tests`, `--with-examples`, `--no-tests`, `--no-examples`, `-h`

### Windows

**Bash** (Git Bash): `build_windows.sh`

```bash
./scripts/build_windows.sh -t Debug -a configure_and_build --dev
./scripts/build_windows.sh -j 8 -t Release --with-examples
```

**Python** (recommended): `build_windows.py` — run from a **Visual Studio Developer Command Prompt**

```bash
python scripts/build_windows.py -t Debug -a configure_and_build --dev
python scripts/build_windows.py --build-type Release --jobs 8 --with-examples
python scripts/build_windows.py --interactive
```

**PowerShell:** `build_windows.ps1`

```powershell
.\scripts\build_windows.ps1 -BuildType Debug -Action configure_and_build -Dev
```

## Documentation

```bash
./scripts/generate-docs.sh
./scripts/generate-docs.sh --api-only
```

## Formatting

```bash
./scripts/format.sh
./scripts/format.sh -check
```

Uses `clang_formatter.py` (same pattern as vnegfx / vneio).

## CI install verification

```bash
cmake --install build/shared --prefix /tmp/vnexr-install --component vnexr
./scripts/ci_verify_xr_install.sh /tmp/vnexr-install shared
```

## Not copied from vnegfx

Gfx-specific tooling was intentionally omitted:

- Shader compilation (`glsl_to_spirv.py`, `convert_shaders.sh`, `generate_shaders.sh`)
- WebGPU prefetch (`ci_prefetch_emdawnwebgpu.sh`)
- Android Vulkan verification (`verify_android_vulkan.sh`)
- Medical volume conversion (`convert_brainweb_volume_to_nrrd.py`)
