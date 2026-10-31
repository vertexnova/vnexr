<p align="center">
  <img src="icons/vertexnova_logo_medallion_with_text.svg" alt="VertexNova Template" width="320"/>
</p>

<p align="center">
  <strong>Minimal C++ project template for the VertexNova ecosystem</strong>
</p>

<p align="center">
  <a href="https://github.com/vertexnova/vnexr/actions/workflows/ci.yml">
    <img src="https://github.com/vertexnova/vnexr/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI"/>
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++ Standard"/>
  <a href="https://codecov.io/gh/vertexnova/vnexr">
    <img src="https://codecov.io/gh/vertexnova/vnexr/branch/main/graph/badge.svg" alt="Coverage"/>
  </a>
  <img src="https://img.shields.io/badge/license-Apache%202.0-green.svg" alt="License"/>
</p>

---

# VneXR

VertexNova **XR session / compositor** library (OpenXR + vnerhi). Learning path (Ubuntu 24.04 -> Monado -> Galaxy XR): [docs/vertexnova/xr/learning_path.md](docs/vertexnova/xr/learning_path.md). Agent playbooks: [skills/](skills/).

## Directory layout

| Path | Description |
|------|-------------|
| `cmake/vnecmake/` | CMake modules submodule (ProjectSetup, ProjectWarnings, VneUseDep) |
| `configs/` | Configured headers (e.g. `config.h.in`) |
| `deps/external/` | Third-party deps (e.g. googletest) |
| `deps/internal/` | VertexNova internal libs (vnecommon, vnelogging) |
| `include/` | Public API headers (`vertexnova/xr/`) |
| `src/` | Implementation |
| `tests/` | Unit tests (Google Test) |
| `docs/` | Doxygen input (`doxyfile.in`) and extra docs |
| `scripts/` | Helper scripts (build, format, generate-docs) |

## Prerequisites

- **CMake** 3.19 or newer  
- **C++20** compiler (e.g. GCC 10+, Clang 10+, MSVC 2019+)  
- **Doxygen** (optional, for `scripts/generate-docs.sh` and `-DENABLE_DOXYGEN=ON`)

## Dependencies

- **External:** Tests use [Google Test](https://github.com/google/googletest). Either add `deps/external/googletest` as a submodule (recommended tag: `v1.17.0`) or let CMake use FetchContent when the directory is missing.  
- **Internal:** **vnecmake** (required) is the CMake modules submodule at `cmake/vnecmake`. Optional libraries `vnecommon` and `vnelogging` go under `deps/internal/`. See [deps/README.md](deps/README.md). If they are missing, the template still builds but does not link to `vne::common` or `vne::logging`.

From the project root:

```bash
git submodule update --init --recursive
```

(Add submodules first if your repo uses them; see `deps/README.md`.)

## Build

Builds use **`build/static`** or **`build/shared`** (one library type per directory). From the project root:

```bash
# Shared library (default)
cmake -B build/shared -DCMAKE_BUILD_TYPE=Debug -DVNE_XR_TESTS=ON
cmake --build build/shared

# Static library
cmake -B build/static -DCMAKE_BUILD_TYPE=Debug -DVNE_XR_LIB_TYPE=static -DVNE_XR_TESTS=ON
cmake --build build/static
```

Or use the platform scripts (they use `build/<lib_type>/...`):

```bash
# macOS (default: shared)
./scripts/build_macos.sh -t Debug -a configure_and_build
./scripts/build_macos.sh -l static -t Release -a configure_and_build   # static in build/static/...

# Linux
./scripts/build_linux.sh -t Debug -a configure_and_build
./scripts/build_linux.sh -l static -c clang -a test

# Linux OpenXR (Monado / desktop VR)
./scripts/install_linux_openxr_deps.sh
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples

# Windows (Visual Studio Developer Command Prompt). Python, PowerShell, and Bash
# share build/<lib>/<config>/build-windows-cl-<cl-version>.
python scripts\build_windows.py -t Debug -a configure_and_build --with-openxr
python scripts\build_windows.py -t Debug -a run --with-openxr --sample null_simulator
python scripts\build_windows.py -t Debug -a run --with-openxr --sample stereo_cube
```

Options: `-t` / `-BuildType` build type, `-a` / `-Action` action, `-l` / `-LibType` lib type (`static` | `shared`, default `shared`), `-clean` / `-Clean`, `-j N` / `-Jobs N`. macOS script also supports `-xcode` for Xcode project.

The Windows scripts additionally build the OpenXR backend with Vulkan (implies samples), run one sample through `vnexr_samples` with `-a run`, and select an OpenXR runtime for that run. Option spelling differs per script. Details: [scripts/README.md](scripts/README.md#windows).

| Purpose | `build_windows.py` / `build_windows.sh` | `build_windows.ps1` |
|---------|-----------------------------------------|---------------------|
| OpenXR backend + Vulkan | `--with-openxr` | `-WithOpenXR` |
| Sample for `-a run` | `--sample <name>` | `-Sample <name>` |
| OpenXR runtime manifest | `--runtime-json <path>` | `-RuntimeJson <path>` |

The Quest 3 walkthrough is in [Windows laptop setup](#windows-laptop-setup) below.

## Windows laptop setup

Two ways to see `stereo_cube` on a Quest 3. Do them in this order: the PC sample through Meta Quest Link, then the on-headset APK. The desktop build does not need Android Studio.

### 1. Tools

Install these before the first compile:

- **Visual Studio 2022** with the Desktop development with C++ workload. `cl` is not on the normal PATH. Use a **Visual Studio Developer Command Prompt**, or call `VsDevCmd.bat` first (`Common7\Tools\VsDevCmd.bat -arch=x64 -host_arch=x64`).
- **CMake** 3.26 or newer, **Python 3**, and **Git for Windows** (the APK script is bash, so Git Bash is required).
- **LunarG Vulkan SDK**, which sets `VULKAN_SDK`. The run step puts `%VULKAN_SDK%\Bin` on `PATH` so the loader DLLs resolve.
- SSH access to the private `vnerhi` repository, then from the repo root: `git submodule update --init --recursive`.

### 2. Desktop OpenXR build

From that Visual Studio prompt, in the repo:

```bat
python scripts\build_windows.py -t Debug -a configure_and_build --with-openxr
python scripts\build_windows.py -t Debug -a run --with-openxr --sample null_simulator
```

`--with-openxr` turns on the OpenXR backend, Vulkan, and the samples. The launcher is `build\shared\Debug\build-windows-cl-*\bin\samples\Debug\vnexr_samples.exe`. Shared DLLs land in `bin\Debug`. `run` prepends that folder and the Vulkan SDK `Bin` directory to `PATH`.

`null_simulator` needs no headset. Success is five `Null simulator frame N` lines and exit code 0. `vnexr_samples.exe --list` includes `stereo_cube`.

### 3. Quest Link (PC sample in the headset)

1. Install the **Meta Quest Link** PC app. A data-capable USB-C cable is the reliable first connection.
2. Put on the Quest 3, accept the Link connection, and stay in the Link home.
3. In the Link app: Settings, General, **Set Meta Quest Link as active OpenXR runtime**.
4. Rerun the sample that is already built:

```bat
python scripts\build_windows.py -t Debug -a run --with-openxr --sample stereo_cube
```

A good log, in order, is `OpenXR Vulkan context ready`, then `OpenXR session initialized`, then frames in the headset. The sample runs until you close it. If the log says the registry has no active runtime, Link is not the active OpenXR runtime. If it says `XR_ERROR_FORM_FACTOR_UNAVAILABLE`, the headset is not in the Link session.

### 4. Android Studio SDK (needed only for the APK)

Install Android Studio, then in SDK Manager install:

- Android SDK Platform **36** and Build-Tools **36.0.0** (the sample compiles against API 36)
- Android SDK Platform-Tools (`adb`)
- NDK (Side by side). Quest 3 is `arm64-v8a`
- CMake (the package includes `ninja.exe`). The Android script uses Ninja. The Visual Studio CMake generator cannot drive the NDK toolchain

The default SDK location is `%LOCALAPPDATA%\Android\Sdk`. The JDK is Android Studio's JBR at `C:\Program Files\Android\Android Studio\jbr`. `scripts/build_android.sh` looks in those places, and also accepts `adb.exe`.

The generated project uses Android Gradle Plugin 9.0.1, which needs **Gradle 9.1 or newer**. The script uses `gradle` from `PATH` (or `GRADLE_DIR`, a project that contains a newer `gradlew`) and stops with a message if it is older. Otherwise it falls back to the newest `gradle.bat` that Android Studio has already downloaded under `%USERPROFILE%\.gradle\wrapper\dists`. On a clean machine install Gradle 9.1 or newer (`brew install gradle` on macOS) and put it on `PATH` so the build does not depend on Android Studio having synced a project.

### 5. Developer mode

1. In the **Meta Horizon** phone app: the Quest 3, headset settings, turn on **Developer mode**. The headset reboots.
2. Connect USB-C, put on the headset, and allow USB debugging.
3. Confirm the PC sees it: `adb devices` shows the headset as `device`, not `unauthorized`.

### 6. Build and install the APK

From **Git Bash** in the repo:

```bash
export ANDROID_HOME="$LOCALAPPDATA/Android/Sdk"
export ANDROID_NDK_ROOT="$(ls -d "$ANDROID_HOME"/ndk/* | sort -V | tail -1)"
./scripts/build_android.sh -t Debug -a run -sample stereo_cube -abi arm64-v8a
```

The two exports are optional when the SDK is in the default Windows location. The script picks the newest installed NDK, configures with Ninja, and on Windows writes the build under `build/android/` (a short folder such as `arm64-v8a-debug-shared-ndk27`, so native object paths stay short enough for Windows). The script prints the exact build folder and APK path. `-a run` installs `com.vertexnova.vnexr.samples` and launches `stereo_cube`; with no `-sample` it opens the in-headset launcher. Expect the app in the headset library. See "Using the app in the headset" below for the controls.

If no headset is connected yet, build the APK without installing:

```bash
./scripts/build_android.sh -t Debug -a apk -sample stereo_cube -abi arm64-v8a
```

The script prints the APK path. It is `app-samples-debug.apk` under `<build folder>/android-studio/app/build/outputs/apk/samples/debug/`. If the view is black, run `./scripts/build_android.sh -a logcat` (it streams the app, OpenXR runtime and crash logs) and capture the output.

### Using the app in the headset

One APK, one XR session. Opening the app shows a launcher panel in front of you. Samples are placed for a seated user: about 50 cm ahead, with a table top about 45 cm below your eyes. If the headset recenters (hold the Meta button, or the floor origin settles after start), the panel or scene is placed in front of you again.

| Action | Control |
|---|---|
| Pick a sample | Point a controller ray at a card and pull the trigger (a hand pinch works too) |
| Back to the launcher | Left controller **≡ Menu** button, or point the right ray at the small **MENU** tag above the left controller and pull the trigger; on the launcher, ≡ resumes the last sample |
| Move the scene | Hold **grip** on one controller and move the hand (the scene moves, it never tilts) |
| Zoom and turn the scene | Hold **both grips**: spread the hands to zoom in, bring them together to zoom out, swing them to turn |
| Turn the scene in steps | Flick the **right thumbstick** left or right (30° each) |
| Reset the view | **B** or **Y**: the scene (or the launcher panel) comes back in front of you, at normal size |
| Close the app | Pull the trigger on the **Exit** card twice within 3 seconds (one stray pull never closes the app), or the **Meta** button and then **Quit** in the system menu |
| stereo_cube | Trigger near a block grabs it (the block in reach glows), trigger in empty space makes a new one, **A/X** recolors the held block; let go and it stands upright and drops onto the table, another block, or the floor |
| Jump straight into a sample | `adb shell am start -n com.vertexnova.vnexr.samples/com.vertexnova.vnexr.samples.VneXrNativeActivity -e sample xr_diagnostics`, or `./scripts/build_android.sh -a run -sample xr_diagnostics`. Starting it again while the app is running switches samples without a restart |

**The scene is too far, too close, or in the wrong place**

1. Press **B** (right controller) or **Y** (left controller). The scene returns in front of you at normal size and height. This also undoes any zoom.
2. To fine-tune, hold **one grip** and pull your hand toward you (or push it away). The scene follows your hand one-to-one and never tilts.
3. If objects look too big or too small rather than too far, hold **both grips** and bring the hands together (smaller) or spread them (larger). Press B/Y to return to normal size.
4. If the scene is off to the side, flick the **right thumbstick** (30° per flick) or press B/Y.
5. If the panel or scene is not visible at all, press **≡ Menu** on the left controller to bring the launcher back, then pick the sample again.

Run **xr_diagnostics** first after any change: a 1 m cube must look single and solid with either eye closed, the grid must stay put when you move your head, each button must light its indicator, and the strip of frame times must stay green. The log prints `XR frame health` every 5 seconds; any non-zero counter in a warning line (frames not drawn, stale poses, swapchain rebuilds) is the first thing to look at when the picture misbehaves.

### Getting logs from the headset

**If a picture looks wrong, run with the Vulkan validation layer first** (section below). Some mistakes, such as a pipeline that does not match the render pass, draw garbage on Quest's GPU without any error in the normal log, and validation names them on the first frame.

Developer mode and a USB cable are all you need. `adb` is in the Android SDK's `platform-tools` (macOS: `~/Library/Android/sdk/platform-tools`, Windows: `%LOCALAPPDATA%\Android\Sdk\platform-tools`); the build script finds it by itself. Check `adb devices` shows the headset as `device` first.

The `adb` commands below need `adb` on your `PATH`; `./scripts/build_android.sh` does not (it finds `adb` itself). If you get `zsh: command not found: adb`, add it once for the terminal you are using (macOS, zsh):

```bash
export PATH="$HOME/Library/Android/sdk/platform-tools:$PATH"
adb devices        # the headset must be listed as "device"
```

To keep it for new terminals: `echo 'export PATH="$HOME/Library/Android/sdk/platform-tools:$PATH"' >> ~/.zshrc`.

**Live log** (leave it running while you use the app, press Ctrl-C to stop):

```bash
./scripts/build_android.sh -a logcat
```

It clears the old log, then streams this app (`vne.xr`), the OpenXR runtime (`OpenXR`, `VrApi`), the Vulkan driver, and native crashes. When you stop it, it prints the crash buffer. To keep a copy for sharing:

```bash
./scripts/build_android.sh -a logcat 2>&1 | tee quest.log
```

**After something went wrong** (the headset is still on and has not rebooted):

```bash
adb logcat -d -v time vne.xr:V OpenXR:V VrApi:V vulkan:V AndroidRuntime:E DEBUG:V libc:F '*:S' > quest.log
adb logcat -b crash -d -v time > quest_crash.log
```

The log buffer is small and is cleared by a reboot, so save it right after the problem. A crash leaves a short summary in `quest_crash.log`; the full native dump (`/data/tombstones`) is not readable on a Quest. For a complete snapshot of the device, run `adb bugreport quest_bugreport.zip`.

**What a healthy start looks like**, in order:

| Log line | Meaning |
|---|---|
| `OpenXR runtime: Oculus ...` | The runtime was found |
| `OpenXR Vulkan device ready` | Vulkan device created through OpenXR |
| `OpenXR session initialized (MSAA request=4, ... reference space=..., color format=..., views=2)` | Session created; this line shows the MSAA you asked for |
| `OpenXR swapchains ready: 2 views ... MSAA x4` | Eye buffers exist; this line shows the MSAA actually in use. `MSAA x1` is expected when `debug.vnexr.msaa` is 1; with a request of 4 it means the 4x allocation failed and the 1x fallback was used |
| `Starting sample '...'` | The sample was attached |
| `XR frame health: N frames in 5s, no problems` | Every 5 seconds while rendering |

A warning version of the last line lists counters: `no-pose` (never tracked), `stale-pose` (tracking dropped, last pose reused), `not-drawn` (a frame was skipped instead of showing garbage), `acquire-fail`, `swapchain-rebuilds`, `wait-fail`, `end-fail`. Any non-zero counter is where to look. `Frame not drawn (...)` errors name the exact reason.

**Rendering switches** (set from the PC, then relaunch the app; they last until the headset reboots). The `OpenXR session initialized` line shows the values in use.

| Property | Default | Effect |
|---|---|---|
| `debug.vnexr.msaa` | `4` | Multisampling of the eye buffers; `1` turns it off |
| `debug.vnexr.sync` | `1` | Wait for the GPU before handing each image to the compositor; `0` skips the wait |
| `debug.vnexr.scale` | `1.25` | Eye buffer size relative to the runtime's recommendation (clamped to its maximum) |
| `debug.vnexr.layerfilter` | `0` | Compositor super sampling (`XR_FB_composition_layer_settings`); `1` turns it on |

Example: `adb shell setprop debug.vnexr.msaa 1`, then quit and relaunch the app.

**Vulkan validation** (finds GPU misuse such as missing synchronization; debug builds only, slows rendering):

```bash
./scripts/build_android.sh -t Debug -a install -validation    # packages the Khronos layer (downloaded once)
adb shell settings put global enable_gpu_debug_layers 1
adb shell settings put global gpu_debug_app com.vertexnova.vnexr.samples
adb shell settings put global gpu_debug_layers VK_LAYER_KHRONOS_validation
```

Relaunch the app; errors appear in `./scripts/build_android.sh -a logcat` under the `VALIDATION` tag. Turn it off again with `adb shell settings delete global enable_gpu_debug_layers` (and the other two keys), or build without `-validation`.

**Text** in the launcher is drawn from a signed distance field font baked at startup from the headset's own Roboto (`/system/fonts/RobotoStatic-Regular.ttf`), so it stays sharp at any size. The log says `Text: using the distance field font`; if no font is found, or the GPU setup fails, it says `Text: using the built-in pixel font` and labels use a blocky fallback. The renderer uses the same atlas conventions and shader math as vnegfx's MSDF text, so a vnegfx atlas can replace the baked one later. `stb_truetype.h` (MIT / public domain) is vendored in `deps/external/stb_truetype` until it moves to vneio.

**Other useful log lines**: `OpenXR session state: ...` (every lifecycle change), `OpenXR interaction profile /user/hand/...: ...` (controllers or hands), `Menu button pressed`, `Wrist MENU tag selected`, `Launcher: ... selected`, `Reference space changed; re-placing ...`, `Snap turn ...`, `Scene navigation: N grip(s) held, scale ...`, `Head pose (...): position (...), heading N deg` (where your head points when something is placed, and every 5 seconds).

**Which lines are ours**: `vne.xr` lines are this app. `VrApi` lines are Meta's runtime, once per second for every VR process; the number in brackets is the process id (ours appears in the `vne.xr` lines, the Meta home shell is another). In ours, `FPS=72/72` and `Stale=0` mean every frame arrived on time.

**Recordings**: a video recorded in the headset (Meta button, Camera, Record video) is saved on it. Copy all of them to the PC with:

```
adb shell ls -lt /sdcard/Oculus/VideoShots/          # newest first; app recordings start with com.vertexnova.vnexr.samples-
mkdir -p ~/Downloads/quest-videos
adb pull /sdcard/Oculus/VideoShots/. ~/Downloads/quest-videos/     # the trailing "/." copies the files directly into the folder
```

Recordings started from the Meta system menu may be named `com.oculus.vrshell-...`; they can still show the app. If `adb` is not found, add `platform-tools` to your `PATH` (see the top of this section).

**The log stops after `OpenXR session initialized`**: the headset has to be worn. Quest only moves the session from `IDLE` to `READY` once the proximity sensor sees a face (or after you cover it, or run `adb shell am broadcast -a com.oculus.vrpowermanager.prox_close` to simulate wearing it, and `prox_far` to undo it). Put the headset on and the log continues.

## Test

```bash
ctest -C Debug --test-dir build/shared
# or for static: ctest -C Debug --test-dir build/static
```

Or:

```bash
./scripts/build_macos.sh -a test
```

## Documentation

- **Template overview and diagrams:** [docs/vertexnova/xr/xr.md](docs/vertexnova/xr/xr.md) — context and API diagrams (Draw.io sources in `docs/vertexnova/xr/diagrams/`).
- **API docs:** Configure with Doxygen enabled and build the doc target:

  ```bash
  cmake -B build/shared -DENABLE_DOXYGEN=ON
  cmake --build build/shared --target vnexr_doc_doxygen
  ```

  Output: `build/shared/docs/html/index.html`.

- **Script:** From project root:

  ```bash
  ./scripts/generate-docs.sh
  ```

  Use `--api-only` to only generate API docs, or `--validate` to only check links and coverage. See `./scripts/generate-docs.sh --help`.

## Format and tidy

- **clang-format:** Config in [.clang-format](.clang-format). Format in place or check only (CI):
  ```bash
  ./scripts/format.sh          # format sources
  ./scripts/format.sh -check   # check only (used in CI)
  ```
- **clang-tidy:** Config in [.clang-tidy](.clang-tidy). Generate `compile_commands.json` (e.g. `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build/shared`), then run `clang-tidy -p build/shared`.

## CI

GitHub Actions runs on push and pull requests to `main`: format check, clang-tidy, and build/test on Linux (GCC, Clang), macOS, and Windows. See [.github/workflows/ci.yml](.github/workflows/ci.yml).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build, test, and style. We follow the [Contributor Covenant](CODE_OF_CONDUCT.md) Code of Conduct.

## Releases

Releases are manual. The **VERSION** file at the repo root is the source of truth; CMake reads it at configure time and exposes it as `getVersion()`.

To cut a release: update **VERSION**, add a dated entry to **CHANGELOG.md**, commit, create and push a tag (e.g. `git tag v1.0.0 && git push origin v1.0.0`), then create a GitHub Release from that tag and paste the CHANGELOG section.

## License

See [LICENSE](LICENSE).
