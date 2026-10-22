# Management demo rehearsal

Target length: **3 minutes** live, with a recorded fallback.

## Checklist (day before)

- [ ] Ubuntu 24.04 laptop charged; Monado starts (`systemctl --user start monado.service`)
- [ ] `sample_02_openxr_stereo` runs on Monado without adb
- [ ] Galaxy XR charged; developer mode on; `adb devices` lists the headset
- [ ] APK installed: `com.vertexnova.vnexr.openxr_stereo`
- [ ] Hand pinch/grasp works in the headset
- [ ] `scrcpy` mirrors to the meeting display
- [ ] Fallback MP4 of a clean run copied to the laptop

## Talk track

1. **Null (10 s)** -- session and layers without hardware.
2. **Monado (60 s)** -- same OpenXR + Vulkan path as the device; backup if USB fails.
3. **Galaxy XR (90 s)** -- production target; bare-hand grab/spawn; mirror via scrcpy.
4. **Optional CSG (30 s)** -- vnegfx content on the session-owned device (Monado).

## Failure plan

| Symptom | Action |
|---------|--------|
| Headset not in `adb devices` | Switch to Monado immediately; fix USB later |
| Black view in HMD | `adb logcat -s vne.xr:* OpenXR:*`; play fallback video |
| No hand input | Controllers still work if paired; mention hand_interaction extension |
| scrcpy fails | Pass the headset around or show the recording |

## Record fallback

```bash
# While the app runs on the headset:
scrcpy --record=vnexr_galaxyxr_demo.mp4 --video-codec=h264
```
