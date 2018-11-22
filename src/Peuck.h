/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <jni.h>

class ChoreographerFilter;
class EGL;

using EGLDisplay = void *;
using EGLSurface = void *;

class Swappy {
  private:
    // Allows construction with std::unique_ptr from a static method, but disallows construction
    // outside of the class since no one else can construct a ConstructorTag
    struct ConstructorTag {
    };
  public:
    Swappy(std::chrono::nanoseconds refreshPeriod,
           std::chrono::nanoseconds appOffset,
           std::chrono::nanoseconds sfOffset,
           ConstructorTag tag);

    static void init(JNIEnv *env, jobject jactivity);

    static void onChoreographer(int64_t frameTimeNanos);

    static bool swap(EGLDisplay display, EGLSurface surface);

    static void init(std::chrono::nanoseconds refreshPeriod,
                     std::chrono::nanoseconds appOffset,
                     std::chrono::nanoseconds sfOffset);

    static void destroyInstance();

private:
    static Swappy *getInstance();

    EGL *getEgl();

    void onSettingsChanged();

    void handleChoreographer();
    std::chrono::nanoseconds wakeClient();

    void startFrame();

    void waitUntil(int32_t frameNumber);

    void waitOneFrame();

    // Waits for the next frame, considering both Choreographer and the prior frame's completion
    void waitForNextFrame(EGLDisplay display);

    // Destroys the previous sync fence (if any) and creates a new one for this frame
    void resetSyncFence(EGLDisplay display);

    // Computes the desired presentation time based on the swap interval and sets it
    // using eglPresentationTimeANDROID
    bool setPresentationTime(EGLDisplay display, EGLSurface surface);

    void updateSwapDuration(std::chrono::nanoseconds duration);
    std::atomic<std::chrono::nanoseconds> mSwapDuration;

    static std::mutex sInstanceMutex;
    static std::unique_ptr<Swappy> sInstance;

    std::atomic<int32_t> mSwapInterval{1};

    std::mutex mWaitingMutex;
    std::condition_variable mWaitingCondition;
    std::chrono::steady_clock::time_point mCurrentFrameTimestamp = std::chrono::steady_clock::now();
    int32_t mCurrentFrame = 0;

    std::mutex mEglMutex;
    std::unique_ptr<EGL> mEgl;

    int32_t mTargetFrame = 0;
    std::chrono::steady_clock::time_point mPresentationTime = std::chrono::steady_clock::now();

    const std::chrono::nanoseconds mRefreshPeriod;
    std::unique_ptr<ChoreographerFilter> mChoreographerFilter;
};
