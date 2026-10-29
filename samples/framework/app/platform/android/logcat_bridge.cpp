/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/platform/android/logcat_bridge.h"

#include <android/log.h>
#include <unistd.h>

#include <cstdio>
#include <string_view>

namespace vne::xr::samples {

namespace {

int priorityFor(std::string_view line) {
    if (line.rfind("[ERROR]", 0) == 0 || line.rfind("[CRITICAL]", 0) == 0) {
        return ANDROID_LOG_ERROR;
    }
    if (line.rfind("[WARN]", 0) == 0) {
        return ANDROID_LOG_WARN;
    }
    return ANDROID_LOG_INFO;
}

}  // namespace

LogcatBridge::LogcatBridge(const char* tag)
    : tag_(tag != nullptr ? tag : "vne.xr") {
    int fds[2] = {-1, -1};
    if (pipe(fds) != 0) {
        return;
    }
    saved_stdout_ = dup(STDOUT_FILENO);
    saved_stderr_ = dup(STDERR_FILENO);
    if (saved_stdout_ < 0 || saved_stderr_ < 0 || dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0) {
        // Put back whatever was redirected before giving up.
        if (saved_stdout_ >= 0) {
            dup2(saved_stdout_, STDOUT_FILENO);
            close(saved_stdout_);
        }
        if (saved_stderr_ >= 0) {
            dup2(saved_stderr_, STDERR_FILENO);
            close(saved_stderr_);
        }
        saved_stdout_ = saved_stderr_ = -1;
        close(fds[0]);
        close(fds[1]);
        return;
    }
    close(fds[1]);  // stdout and stderr now hold the write end.
    read_fd_ = fds[0];
    // Without this the pipe is block-buffered and lines would reach logcat late.
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IOLBF, 0);
    is_active_ = true;
    reader_ = std::thread([this] { readLoop(); });
}

LogcatBridge::~LogcatBridge() {
    if (!is_active_) {
        return;
    }
    std::fflush(stdout);
    std::fflush(stderr);
    // Restoring the original descriptors closes the last write ends, which ends the reader with EOF.
    dup2(saved_stdout_, STDOUT_FILENO);
    dup2(saved_stderr_, STDERR_FILENO);
    close(saved_stdout_);
    close(saved_stderr_);
    if (reader_.joinable()) {
        reader_.join();
    }
    close(read_fd_);
}

void LogcatBridge::readLoop() {
    std::string pending;
    char buffer[1024];
    const auto flush_line = [this, &pending]() {
        if (!pending.empty()) {
            __android_log_write(priorityFor(pending), tag_.c_str(), pending.c_str());
            pending.clear();
        }
    };
    while (true) {
        const ssize_t count = read(read_fd_, buffer, sizeof(buffer));
        if (count <= 0) {
            break;
        }
        for (ssize_t i = 0; i < count; ++i) {
            if (buffer[i] == '\n') {
                flush_line();
            } else {
                pending.push_back(buffer[i]);
            }
        }
    }
    flush_line();
}

}  // namespace vne::xr::samples
