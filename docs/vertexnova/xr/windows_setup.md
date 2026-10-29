# Windows setup (first OpenXR run)

Windows is the quickest place to run vnexr against a real OpenXR runtime: a Quest 3 connected with **Meta Quest
Link** acts as a PC VR headset, and the **Meta XR Simulator** works without any headset. The Android APK for the
headset itself comes after this works.

## Prerequisites

- Visual Studio 2022 (Desktop development with C++), CMake 3.26+, Python 3, Git (with Git Bash).
- LunarG **Vulkan SDK** (sets `VULKAN_SDK`).
- SSH access to the private `vnerhi` repository.

## Get the sources

From Git Bash in the repository root:

```bash
./scripts/init_submodules.sh full
```

## Build

From a **Visual Studio Developer Command Prompt**:

```bat
python scripts\build_windows.py -t Debug -a configure_and_build --with-openxr
```

`--with-openxr` turns on the OpenXR backend and Vulkan and builds the samples. The launcher ends up in
`build\shared\Debug\build-windows-cl-*\bin\samples\Debug\vnexr_samples.exe`.

## Run without a runtime first

```bat
python scripts\build_windows.py -t Debug -a run --sample null_simulator
```

Expect five `Null simulator frame N` lines and exit code 0. `vnexr_samples.exe --list` shows every sample.

## Option A: Quest 3 through Meta Quest Link

1. Install the **Meta Quest Link** app on the PC, put the headset on, and start Link (USB-C cable or Air Link).
2. In the Link app open Settings, General, and choose **Set Meta Quest Link as active OpenXR runtime**.
3. Keep the headset in the Link home environment, then run:

```bat
python scripts\build_windows.py -t Debug -a run --sample stereo_cube
```

## Option B: Meta XR Simulator (no headset)

Install the Meta XR Simulator, find its OpenXR manifest (`meta_openxr_simulator.json`, inside its install folder),
and pass it for this run only:

```bat
python scripts\build_windows.py -t Debug -a run --sample stereo_cube --runtime-json "C:\path\to\meta_openxr_simulator.json"
```

## What a good run looks like

The log shows, in order:

1. `OpenXR Vulkan context ready (enable2=1, min Vulkan=...)`
2. `OpenXR session initialized (enable2=1)`
3. the session reaching READY, then frames rendering in the headset or simulator window.

Closing the window (or the sample's own exit) must end the process cleanly with exit code 0.

## If it fails

Send the full console output. These are the usual causes:

| Symptom | Likely cause |
|---------|--------------|
| `XR_ERROR_RUNTIME_UNAVAILABLE` or no runtime found | No active OpenXR runtime: do the Link step above, or pass `--runtime-json` |
| `XR_ERROR_FORM_FACTOR_UNAVAILABLE` | The runtime is active but the headset or simulator is not running |
| `Vulkan loader supports up to ... requires at least ...` | An old GPU driver: update it |
| CMake cannot find Vulkan | `VULKAN_SDK` is not set in this prompt |
| Black or flickering eyes | Capture the console log; this is the first real swapchain run, so the log is the main clue |

## Then the headset itself

Enable developer mode (Meta Horizon phone app), connect the Quest 3 over USB-C, accept the debugging prompt in the
headset, and check `adb devices`. APK builds are done from macOS or Windows with `scripts/build_android.sh`
(see [learning_path.md](learning_path.md)).
