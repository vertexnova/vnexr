/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

package com.vertexnova.vnexr.samples;

import android.content.Intent;
import android.os.Bundle;

/**
 * NativeActivity that keeps the latest launch intent visible to native code.
 *
 * <p>launchMode singleTask delivers a second start to {@link #onNewIntent}. Stock NativeActivity
 * does not replace the intent returned by {@link #getIntent()} and does not notify the native
 * thread, so the sample launcher reads {@link #getLatestIntent()} instead.
 */
public final class VneXrNativeActivity extends android.app.NativeActivity {
    private static volatile Intent latestIntent;
    /** Starts at 1 so a native reader can tell "no tracker" (0) from the initial launch. */
    private static volatile int latestGeneration = 1;

    public static Intent getLatestIntent() {
        return latestIntent;
    }

    public static int getLatestGeneration() {
        return latestGeneration;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Publish before super.onCreate: that call loads native code and starts android_main.
        latestIntent = getIntent();
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        // Intent first, then generation, so a reader that observes the new generation also sees this intent.
        latestIntent = intent;
        latestGeneration = latestGeneration + 1;
    }
}
