/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file android_main.cpp
 * @brief NativeActivity entry: one XR session that runs whichever sample is picked.
 *
 * Without a launch extra the in-headset launcher panel opens. To jump straight into a sample:
 *
 *   adb shell am start -n <package>/com.vertexnova.vnexr.samples.VneXrNativeActivity -e sample stereo_cube
 *
 * The activity is singleTask: a later start arrives through onNewIntent and switches the running app to that sample
 * (or back to the launcher) without restarting the XR session.
 */

#include "app/platform/android/logcat_bridge.h"
#include "app/sample_registry.h"
#include "app/xr_sample_host.h"
#include "framework/common/logging_guard.h"

#include <android/api-level.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include <jni.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>

namespace {

/** Attaches this thread to the VM for the object's lifetime. */
class JniThreadAttach {
   public:
    explicit JniThreadAttach(JavaVM* vm)
        : vm_(vm) {
        if (vm_ == nullptr || vm_->AttachCurrentThread(&env_, nullptr) != JNI_OK) {
            env_ = nullptr;
        }
    }

    ~JniThreadAttach() {
        if (env_ != nullptr && vm_ != nullptr) {
            vm_->DetachCurrentThread();
        }
    }

    JniThreadAttach(const JniThreadAttach&) = delete;
    JniThreadAttach& operator=(const JniThreadAttach&) = delete;

    [[nodiscard]] JNIEnv* env() const { return env_; }

   private:
    JavaVM* vm_ = nullptr;
    JNIEnv* env_ = nullptr;
};

/**
 * Answers "has the launch intent changed?" without per-call lookups: the activity class and the
 * `getLatestGeneration` method ID are resolved once, so each frame costs a single static call.
 */
class GenerationProbe {
   public:
    GenerationProbe(JNIEnv* env, jobject activity)
        : env_(env) {
        if (env_ == nullptr || activity == nullptr) {
            return;
        }
        jclass local_class = env_->GetObjectClass(activity);
        if (local_class == nullptr) {
            env_->ExceptionClear();
            return;
        }
        method_ = env_->GetStaticMethodID(local_class, "getLatestGeneration", "()I");
        if (env_->ExceptionCheck()) {
            env_->ExceptionClear();
            method_ = nullptr;
        }
        if (method_ != nullptr) {
            class_ = static_cast<jclass>(env_->NewGlobalRef(local_class));
        }
        env_->DeleteLocalRef(local_class);
    }

    ~GenerationProbe() {
        if (class_ != nullptr) {
            env_->DeleteGlobalRef(class_);
        }
    }

    GenerationProbe(const GenerationProbe&) = delete;
    GenerationProbe& operator=(const GenerationProbe&) = delete;

    /** The current generation, or 0 when the activity does not publish one. */
    [[nodiscard]] int read() const {
        if (class_ == nullptr || method_ == nullptr) {
            return 0;
        }
        const int generation = env_->CallStaticIntMethod(class_, method_);
        if (env_->ExceptionCheck()) {
            env_->ExceptionClear();
            return 0;
        }
        return generation;
    }

   private:
    JNIEnv* env_ = nullptr;
    jclass class_ = nullptr;  //!< Global ref, released in the destructor.
    jmethodID method_ = nullptr;
};

/** Intent to select a sample from, plus a generation that changes on each onNewIntent. */
struct LaunchIntent {
    jobject intent = nullptr;  //!< Local ref; the caller deletes it.
    int generation = 0;        //!< 0 when the activity does not publish onNewIntent.
};

jobject activityGetIntent(JNIEnv* env, jobject activity) {
    jclass activity_class = env->GetObjectClass(activity);
    if (activity_class == nullptr) {
        env->ExceptionClear();
        return nullptr;
    }
    jmethodID get_intent = env->GetMethodID(activity_class, "getIntent", "()Landroid/content/Intent;");
    if (get_intent == nullptr || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(activity_class);
        return nullptr;
    }
    jobject intent = env->CallObjectMethod(activity, get_intent);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (intent != nullptr) {
            env->DeleteLocalRef(intent);
        }
        intent = nullptr;
    }
    env->DeleteLocalRef(activity_class);
    return intent;
}

/** Latest intent from VneXrNativeActivity, or the activity's original intent when that class is not used. */
LaunchIntent readLaunchIntent(android_app* app, JNIEnv* env) {
    LaunchIntent launch;
    if (env == nullptr || app == nullptr || app->activity == nullptr || app->activity->clazz == nullptr) {
        return launch;
    }
    jobject activity = app->activity->clazz;
    jclass activity_class = env->GetObjectClass(activity);
    if (activity_class == nullptr) {
        env->ExceptionClear();
        return launch;
    }

    jmethodID get_latest = env->GetStaticMethodID(activity_class, "getLatestIntent", "()Landroid/content/Intent;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        get_latest = nullptr;
    }
    jmethodID get_generation = env->GetStaticMethodID(activity_class, "getLatestGeneration", "()I");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        get_generation = nullptr;
    }

    const bool tracked = get_latest != nullptr && get_generation != nullptr;
    if (tracked) {
        // Generation first: the activity stores the intent before bumping the generation.
        launch.generation = env->CallStaticIntMethod(activity_class, get_generation);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            launch.generation = 0;
        } else {
            launch.intent = env->CallStaticObjectMethod(activity_class, get_latest);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                if (launch.intent != nullptr) {
                    env->DeleteLocalRef(launch.intent);
                    launch.intent = nullptr;
                }
                launch.generation = 0;
            }
        }
    }
    env->DeleteLocalRef(activity_class);

    if (launch.intent == nullptr) {
        launch.intent = activityGetIntent(env, activity);
        if (!tracked) {
            launch.generation = 0;
        }
    }
    return launch;
}

/** The string extra @p key of @p intent, or an empty string. */
std::string readIntentStringExtra(JNIEnv* env, jobject intent, const char* key) {
    if (env == nullptr || intent == nullptr || key == nullptr) {
        return {};
    }
    std::string value;
    jclass intent_class = env->GetObjectClass(intent);
    if (intent_class == nullptr) {
        env->ExceptionClear();
        return {};
    }
    jmethodID get_string_extra =
        env->GetMethodID(intent_class, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
    if (get_string_extra == nullptr || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(intent_class);
        return {};
    }
    jstring java_key = env->NewStringUTF(key);
    if (java_key == nullptr || env->ExceptionCheck()) {
        env->ExceptionClear();
        if (java_key != nullptr) {
            env->DeleteLocalRef(java_key);
        }
        env->DeleteLocalRef(intent_class);
        return {};
    }
    auto java_value = static_cast<jstring>(env->CallObjectMethod(intent, get_string_extra, java_key));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (java_value != nullptr) {
            env->DeleteLocalRef(java_value);
        }
        java_value = nullptr;
    }
    if (java_value != nullptr) {
        const char* chars = env->GetStringUTFChars(java_value, nullptr);
        if (chars != nullptr) {
            value = chars;
            env->ReleaseStringUTFChars(java_value, chars);
        }
        env->DeleteLocalRef(java_value);
    }
    env->DeleteLocalRef(java_key);
    env->DeleteLocalRef(intent_class);
    return value;
}

/**
 * Debug override from `adb shell setprop <name> <value>` (survives until the headset reboots). Lets one APK test
 * MSAA and GPU synchronization on the headset without a rebuild.
 */
int readDebugInt(const char* name, int fallback) {
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get(name, value) <= 0) {
        return fallback;
    }
    return std::atoi(value);
}

float readDebugFloat(const char* name, float fallback) {
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get(name, value) <= 0) {
        return fallback;
    }
    return std::strtof(value, nullptr);
}

/** State the looper callback and the per-frame pump share. */
struct PumpState {
    android_app* app = nullptr;
    JNIEnv* env = nullptr;
    const GenerationProbe* probe = nullptr;
    vne::xr::samples::XrSampleHost* host = nullptr;
    int generation = 0;
    /** Tracks APP_CMD_RESUME / APP_CMD_PAUSE. The activity is resumed when android_main starts. */
    bool is_resumed = true;
};

void handleAppCmd(android_app* app, int32_t command) {
    auto* state = static_cast<PumpState*>(app->userData);
    if (state == nullptr) {
        return;
    }
    switch (command) {
        case APP_CMD_RESUME:
            state->is_resumed = true;
            break;
        case APP_CMD_PAUSE:
            state->is_resumed = false;
            break;
        default:
            break;
    }
}

/**
 * Drain the Android looper. While the activity is paused and the XR session is not running there is nothing to
 * render, so wait (bounded) for the next event instead of spinning.
 */
bool pumpAndroidEvents(android_app* app, bool is_idle) {
    constexpr int kIdleWaitMs = 100;
    int timeout = is_idle ? kIdleWaitMs : 0;
    int events = 0;
    android_poll_source* source = nullptr;
    while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source != nullptr) {
            source->process(app, source);
        }
        timeout = 0;
    }
    return app->destroyRequested == 0;
}

/** Pumps events and forwards a new launch intent to the host as a sample switch. */
bool pumpAndWatchIntent(PumpState& state, vne::xr::ISession& session) {
    const bool is_idle = !state.is_resumed && !session.isSessionRunning();
    if (!pumpAndroidEvents(state.app, is_idle)) {
        return false;
    }
    if (state.generation == 0 || state.env == nullptr || state.probe == nullptr || state.host == nullptr) {
        return true;
    }
    // Cheap per-frame check; the full intent read below only happens when something changed.
    const int current_generation = state.probe->read();
    if (current_generation == 0 || current_generation == state.generation) {
        return true;
    }
    LaunchIntent latest = readLaunchIntent(state.app, state.env);
    const bool changed = latest.generation != 0 && latest.generation != state.generation;
    if (changed) {
        const std::string requested = readIntentStringExtra(state.env, latest.intent, "sample");
        VNE_LOG_INFO << "Launch intent requests '" << (requested.empty() ? "launcher" : requested) << "'";
        state.host->requestSample(requested);
        state.generation = latest.generation;
    }
    if (latest.intent != nullptr) {
        state.env->DeleteLocalRef(latest.intent);
    }
    return true;
}

}  // namespace

void android_main(android_app* app) {
    // First, so every log line (including the logging guard's own) reaches `adb logcat -s vne.xr:*`.
    vne::xr::samples::LogcatBridge logcat("vne.xr");
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerAllSamples(registry);

    JniThreadAttach jni(app->activity->vm);
    JNIEnv* env = jni.env();

    PumpState pump_state;
    pump_state.app = app;
    pump_state.env = env;
    app->userData = &pump_state;
    app->onAppCmd = handleAppCmd;

    if (env != nullptr) {
        const GenerationProbe generation_probe(env, app->activity->clazz);
        LaunchIntent launch = readLaunchIntent(app, env);
        const std::string requested = readIntentStringExtra(env, launch.intent, "sample");
        if (launch.intent != nullptr) {
            env->DeleteLocalRef(launch.intent);
        }

        vne::xr::SessionConfig config;
        config.backend = vne::xr::BackendType::eOpenXr;
        config.application_name = "vnexr";
        config.platform.application_vm = app->activity->vm;
        config.platform.application_context = app->activity->clazz;
        // adb shell setprop debug.vnexr.msaa 1     -> no multisampling (default 4)
        // adb shell setprop debug.vnexr.sync 0     -> do not wait for the GPU before every image release (default 1)
        config.color_sample_count = static_cast<std::uint32_t>(std::max(1, readDebugInt("debug.vnexr.msaa", 4)));
        config.is_gpu_sync_every_frame = readDebugInt("debug.vnexr.sync", 1) != 0;
        // adb shell setprop debug.vnexr.scale 1.0       -> eye buffers at the runtime's recommended size (default 1.25)
        // adb shell setprop debug.vnexr.layerfilter 1   -> compositor super sampling (default 0)
        config.resolution_scale = readDebugFloat("debug.vnexr.scale", config.resolution_scale);
        config.use_compositor_supersampling = readDebugInt("debug.vnexr.layerfilter", 0) != 0;

        vne::xr::samples::XrSampleHost host(registry, config);
        host.setInitialSample(requested);
        pump_state.probe = &generation_probe;
        pump_state.host = &host;
        pump_state.generation = launch.generation;
        host.setFramePump(
            [&pump_state](vne::xr::ISession& session) { return pumpAndWatchIntent(pump_state, session); });
        static_cast<void>(host.run());
        pump_state.host = nullptr;
    } else {
        VNE_LOG_ERROR << "Could not attach the sample thread to the Java VM";
    }
    app->userData = nullptr;
    ANativeActivity_finish(app->activity);
}
