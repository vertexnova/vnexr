# Sample architecture

Samples share a thin framework under `samples/framework/`:

```
main / android_main
  -> XrSampleApp (owns ISession, implements IRenderSession)
       -> XrLayerStack
            -> scene layers (e.g. StereoCubeSceneLayer)
            -> overlay layers (optional)
```

Each frame:

1. Session `beginFrame` fills `Frame` (views + `SurfaceTextures`).
2. `XrSampleApp::update` builds `FrameParams` and calls `XrLayerStack::onUpdate`.
3. Layers draw with `session->graphicsDevice()` into per-eye textures.
4. Session `endFrame` submits composition layers.

## Adding a sample

1. Subclass `IXrLayer` in a numbered folder (`NN_name`).
2. `app.pushLayer(std::make_unique<MyLayer>())`.
3. Desktop: `return app.run()`. Android: NativeActivity entry in `main.cpp` (or shared platform entry).
4. Register with CMake; on Android, `vne_xr_register_android_sample` for APK flavors.

See [platforms.md](platforms.md) and [../vertexnova/xr/learning_path.md](../vertexnova/xr/learning_path.md).
