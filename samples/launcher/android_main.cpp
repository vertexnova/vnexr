/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file android_main.cpp
 * @brief NativeActivity entry: runs the sample named by the launch intent extra "sample".
 *
 *   adb shell am start -n <package>/com.vertexnova.vnexr.samples.VneXrNativeActivity -e sample stereo_cube
 *
 * Without the extra, the first OpenXR sample runs so a headset-library launch is immersive.
 * The activity is singleTask: a later start arrives through onNewIntent and replaces the running sample.
 */

#include "app/sample_launcher.h"
#include "app/sample_registry.h"
#include "framework/common/logging_guard.h"

#include <android_native_app_glue.h>
#include <jni.h>

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
 * Explicit names go through find(). An empty request picks the first OpenXR sample so a headset
 * launch does not run the null-backend sample registered first.
 */
const vne::xr::samples::SampleInfo* selectSample(const vne::xr::samples::SampleRegistry& registry,
                                                 std::string_view requested) {
    if (!requested.empty()) {
        return registry.find(requested);
    }
    for (const vne::xr::samples::SampleInfo& info : registry.samples()) {
        if (info.backend == vne::xr::BackendType::eOpenXr) {
            return &info;
        }
    }
    return registry.defaultSample();
}

/** Drain the Android looper without blocking: the session loop supplies its own idle wait. */
bool pumpAndroidEvents(android_app* app) {
    int events = 0;
    android_poll_source* source = nullptr;
    while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source != nullptr) {
            source->process(app, source);
        }
    }
    return app->destroyRequested == 0;
}

struct PumpState {
    android_app* app = nullptr;
    JNIEnv* env = nullptr;
    const vne::xr::samples::SampleRegistry* registry = nullptr;
    int generation = 0;
    bool switch_requested = false;
};

/** Pumps events and stops the session when onNewIntent names a sample to run instead. */
bool pumpAndWatchIntent(PumpState& state, vne::xr::ISession&) {
    if (!pumpAndroidEvents(state.app)) {
        return false;
    }
    if (state.generation == 0 || state.env == nullptr || state.registry == nullptr) {
        return true;
    }
    LaunchIntent latest = readLaunchIntent(state.app, state.env);
    const bool changed = latest.generation != 0 && latest.generation != state.generation;
    if (!changed) {
        if (latest.intent != nullptr) {
            state.env->DeleteLocalRef(latest.intent);
        }
        return true;
    }
    const std::string requested = readIntentStringExtra(state.env, latest.intent, "sample");
    const vne::xr::samples::SampleInfo* next = selectSample(*state.registry, requested);
    if (latest.intent != nullptr) {
        state.env->DeleteLocalRef(latest.intent);
    }
    if (next == nullptr) {
        VNE_LOG_ERROR << "No sample named '" << requested << "'; keeping the current sample";
        state.generation = latest.generation;
        return true;
    }
    VNE_LOG_INFO << "Launch intent now requests '" << (requested.empty() ? next->name : requested) << "'";
    state.switch_requested = true;
    return false;
}

}  // namespace

void android_main(android_app* app) {
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerAllSamples(registry);

    JniThreadAttach jni(app->activity->vm);
    JNIEnv* env = jni.env();

    vne::xr::SessionPlatformHandle platform;
    platform.application_vm = app->activity->vm;
    platform.application_context = app->activity->clazz;

    if (env != nullptr) {
        while (app->destroyRequested == 0) {
            LaunchIntent launch = readLaunchIntent(app, env);
            const std::string requested = readIntentStringExtra(env, launch.intent, "sample");
            const int generation = launch.generation;
            if (launch.intent != nullptr) {
                env->DeleteLocalRef(launch.intent);
            }

            const vne::xr::samples::SampleInfo* sample = selectSample(registry, requested);
            if (sample == nullptr) {
                VNE_LOG_ERROR << "No sample named '" << requested << "'; nothing to run";
                break;
            }

            PumpState pump_state;
            pump_state.app = app;
            pump_state.env = env;
            pump_state.registry = &registry;
            pump_state.generation = generation;
            static_cast<void>(
                vne::xr::samples::launchSample(*sample, platform, [&pump_state](vne::xr::ISession& session) {
                    return pumpAndWatchIntent(pump_state, session);
                }));
            if (app->destroyRequested != 0 || !pump_state.switch_requested) {
                break;
            }
        }
    } else {
        VNE_LOG_ERROR << "Could not attach the sample thread to the Java VM";
    }
    ANativeActivity_finish(app->activity);
}
