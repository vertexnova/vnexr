# Learning path (Ubuntu 24.04 first)

Hands-on ladder from Null simulator to Galaxy XR. Each step has commands, what
you should see, the common failure, and when to stop and debug.

Primary machine: **Ubuntu 24.04**. macOS can do steps 1 and 3 only (OpenXR is
blocked on Apple; use visionOS for that platform).

## Step 1 -- Null session (no headset)

**Goal:** session + layer loop without an OpenXR runtime.

```bash
./scripts/init_submodules.sh full
./scripts/build_linux.sh -t Debug -a configure_and_build --dev
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_01_null_simulator
```

**Expect:** process runs briefly and exits cleanly; logs show null backend frames.

**Common failure:** missing vnerhi. Fix: `export VNE_VNERHI_PATH=../vnerhi` and re-init.

**Stop if:** the Null sample does not link. Do not continue to OpenXR yet.

## Step 2 -- Monado stereo cubes

**Goal:** OpenXR + Vulkan on desktop; interactive cubes (keyboard/mouse via Monado).

```bash
./scripts/install_linux_openxr_deps.sh
./scripts/build_linux.sh -t Debug -a configure_and_build --with-openxr --with-examples
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
export QWERTY_ENABLE=1
export XRT_DEBUG_GUI=1
systemctl --user start monado.service
# or: QWERTY_ENABLE=1 XRT_DEBUG_GUI=1 monado-service &
./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo
```

`QWERTY_ENABLE=1` and `XRT_DEBUG_GUI=1` start the virtual HMD window and
keyboard/mouse controllers for a no-headset run.

**Expect:** windowed compositor shows stereo views; grab/spawn with Monado qwerty bindings.

**Common failure:** `XR_ERROR_FORM_FACTOR_UNAVAILABLE` -- Monado not running or wrong JSON path.
Find the manifest: `ls /usr/share/openxr/1/` and `ls /etc/xdg/openxr/1/`.

**Stop if:** session never reaches synchronized/visible. Fix runtime before Android.

## Step 3 -- One device rule

**Goal:** understand why samples never create a second Vulkan device.

Read:

- [session.h](../../../include/vertexnova/xr/session.h) -- `graphicsDevice()`
- [swapchain_image.h](../../../include/vertexnova/xr/swapchain_image.h) -- per-eye textures
- [openxr_design.md](openxr_design.md) -- bootstrap + ownership
- Skill: [vne-xr-session](../../../skills/vne-xr-session/SKILL.md)

**Expect:** you can explain: host creates device via enable2; sample draws into `Frame.surfaces`.

**Stop if:** unclear -- re-read the Mermaid sequence in `openxr_design.md` before packaging.

## Step 4 -- Galaxy XR APK

**Goal:** same sample on the headset with bare-hand pinch/grasp.

Pick an installed NDK under `$ANDROID_HOME/ndk/` (example: `27.2.12479018`), then:

```bash
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk/$(ls "$ANDROID_HOME/ndk" | sort -V | tail -1)
./scripts/build_android.sh -t Debug -a install \
  -sample sample_02_openxr_stereo -abi arm64-v8a
```

Enable developer mode on Galaxy XR; confirm `adb devices` shows the headset.

**Expect:** app appears in the library; stereo cubes; pinch spawns, grasp grabs.

**Common failure:** install succeeds but black view -- check logcat for swapchain/format errors.
Missing hand response -- confirm `XR_EXT_hand_interaction` is enabled (see input module).

**Stop if:** APK does not install. Fix NDK/SDK and Gradle before demo day.

## Step 5 -- Debug + mirror

```bash
./scripts/verify_android_vulkan.sh com.vertexnova.vnexr.openxr_stereo
adb logcat -s vne.xr:* OpenXR:*
scrcpy --video-codec=h264 --max-size 1280
```

**Expect:** mirror on the laptop for the audience; clean log with no XR_ERROR spam.

Record a fallback video of a clean run for the management demo.

## Step 6 -- Stretch: CSG session scaffold (Monado)

**Goal (optional):** exercise `GfxSession` on the XR session device. Stereo CSG
solids are **future work**; today `sample_03_openxr_csg` attaches the gfx
session, counts frames, and submits **zero** projection layers (no solids drawn).

```bash
# Requires vnegfx beside vnexr and matching vnerhi pin
cmake -B build/csg -DVNE_XR_WITH_OPENXR=ON -DVNE_XR_WITH_GFX_SAMPLE=ON \
  -DVNE_XR_SAMPLES=ON -DVNE_XR_DEV=ON
cmake --build build/csg -j$(nproc)
./build/csg/.../bin/samples/sample_03_openxr_csg
```

**Expect:** process runs under Monado; logs say frame loop live with zero projection layers.

**Stop if:** gfx and XR disagree on the device. Use `GfxSession(IDevice&)` with
`session->graphicsDevice()` only -- never the C offscreen API (it owns its own device).

## Demo talk track (3 minutes)

1. Null -- "session and layers without a headset" (10 s)
2. Monado -- "same code path as the device, desktop backup" (60 s)
3. Galaxy XR -- "production OpenXR + hand input" (90 s)
4. Optional CSG scaffold -- "GfxSession on the session device; solids TBD" (30 s)
