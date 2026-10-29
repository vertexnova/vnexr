#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include <string>
#include <thread>

namespace vne::xr::samples {

/**
 * @brief Sends everything written to stdout and stderr to Android's logcat.
 *
 * A NativeActivity's stdout goes nowhere, so console logging is invisible to `adb logcat`. While this object
 * lives, both streams are redirected into a pipe and a reader thread forwards each line with `__android_log_write`.
 * Lines starting with `[ERROR]` or `[WARN]` (the vnelogging console pattern) keep their severity.
 */
class LogcatBridge {
   public:
    /** @param tag logcat tag, e.g. "vne.xr" (filter with `adb logcat -s vne.xr:*`). */
    explicit LogcatBridge(const char* tag);
    ~LogcatBridge();

    LogcatBridge(const LogcatBridge&) = delete;
    LogcatBridge& operator=(const LogcatBridge&) = delete;

    /** @brief False when the redirect could not be set up; output then stays where it was. */
    [[nodiscard]] bool isActive() const noexcept { return is_active_; }

   private:
    void readLoop();

    std::string tag_;
    int read_fd_ = -1;
    int saved_stdout_ = -1;
    int saved_stderr_ = -1;
    bool is_active_ = false;
    std::thread reader_;
};

}  // namespace vne::xr::samples
