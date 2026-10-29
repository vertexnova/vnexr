/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file android_main.cpp
 * @brief NativeActivity entry: runs the sample named by the launch intent extra "sample".
 *
 *   adb shell am start -n <package>/android.app.NativeActivity -e sample stereo_cube
 *
 * Without the extra, the first registered sample runs.
 */

#include "app/sample_launcher.h"
#include "app/sample_registry.h"
#include "framework/common/logging_guard.h"

#include <android_native_app_glue.h>
#include <jni.h>

#include <string>

namespace {

/** The string extra @p key of the activity's launch intent, or an empty string. */
std::string readIntentStringExtra(android_app* app, const char* key) {
    JNIEnv* env = nullptr;
    if (app->activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK || env == nullptr) {
        return {};
    }
    std::string value;
    jobject activity = app->activity->clazz;
    jclass activity_class = env->GetObjectClass(activity);
    jmethodID get_intent = env->GetMethodID(activity_class, "getIntent", "()Landroid/content/Intent;");
    jobject intent = get_intent != nullptr ? env->CallObjectMethod(activity, get_intent) : nullptr;
    if (intent != nullptr) {
        jclass intent_class = env->GetObjectClass(intent);
        jmethodID get_string_extra =
            env->GetMethodID(intent_class, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
        jstring java_key = env->NewStringUTF(key);
        auto java_value = get_string_extra != nullptr
                              ? static_cast<jstring>(env->CallObjectMethod(intent, get_string_extra, java_key))
                              : nullptr;
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
        env->DeleteLocalRef(intent);
    }
    env->DeleteLocalRef(activity_class);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    app->activity->vm->DetachCurrentThread();
    return value;
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

}  // namespace

void android_main(android_app* app) {
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerAllSamples(registry);

    const std::string requested = readIntentStringExtra(app, "sample");
    const vne::xr::samples::SampleInfo* sample =
        requested.empty() ? registry.defaultSample() : registry.find(requested);
    if (sample == nullptr) {
        VNE_LOG_ERROR << "No sample named '" << requested << "'; nothing to run";
        ANativeActivity_finish(app->activity);
        return;
    }

    vne::xr::SessionPlatformHandle platform;
    platform.application_vm = app->activity->vm;
    platform.application_context = app->activity->clazz;

    static_cast<void>(vne::xr::samples::launchSample(*sample, platform, [app](vne::xr::ISession&) {
        return pumpAndroidEvents(app);
    }));
    ANativeActivity_finish(app->activity);
}
