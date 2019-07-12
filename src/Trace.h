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

#include <dlfcn.h>
#include <memory>

#include <android/log.h>
#include <android/trace.h>

class Trace {
  public:
    using ATrace_beginSection_type = void (*)(const char *sectionName);
    using ATrace_endSection_type = void (*)();
    using ATrace_isEnabled_type = bool (*)();

    Trace() {
        __android_log_print(ANDROID_LOG_INFO, "Trace", "Unable to load NDK tracing APIs");
    }

    Trace(ATrace_beginSection_type beginSection,
          ATrace_endSection_type endSection,
          ATrace_isEnabled_type isEnabled)
        : ATrace_beginSection(beginSection),
          ATrace_endSection(endSection),
          ATrace_isEnabled(isEnabled) {}

    static Trace* create() {
        void *libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        if (!libandroid) {
            return 0;
        }

        auto beginSection = reinterpret_cast<ATrace_beginSection_type>(
            dlsym(libandroid, "ATrace_beginSection"));
        if (!beginSection) {
            return 0;
        }

        auto endSection = reinterpret_cast<ATrace_endSection_type>(
            dlsym(libandroid, "ATrace_endSection"));
        if (!endSection) {
            return 0;
        }

        auto isEnabled = reinterpret_cast<ATrace_isEnabled_type>(
            dlsym(libandroid, "ATrace_isEnabled"));
        if (!isEnabled) {
            return 0;
        }

        return new Trace(beginSection, endSection, isEnabled);
    }

    bool isAvailable() const {
        return ATrace_beginSection != 0;
    }

    bool isEnabled() const {
        return (ATrace_isEnabled != 0) && ATrace_isEnabled();
    }

    void beginSection(const char *name) const {
        if (!ATrace_beginSection) {
            return;
        }

        ATrace_beginSection(name);
    }

    void endSection() const {
        if (!ATrace_endSection) {
            return;
        }

        ATrace_endSection();
    }

    static Trace *getInstance() {
        static Trace* trace = Trace::create();
        return trace;
    };

  private:
    const ATrace_beginSection_type ATrace_beginSection = 0;
    const ATrace_endSection_type ATrace_endSection = 0;
    const ATrace_isEnabled_type ATrace_isEnabled = 0;
};

struct ScopedTrace {
    ScopedTrace(const char *name) {
        Trace *trace = Trace::getInstance();
        if (!trace->isAvailable() || !trace->isEnabled()) {
            return;
        }

        trace->beginSection(name);
        mIsTracing = true;
    }

    ~ScopedTrace() {
        if (!mIsTracing) {
            return;
        }

        Trace *trace = Trace::getInstance();
        trace->endSection();
    }

  private:
    bool mIsTracing = false;
};

#define PASTE_HELPER_HELPER(a, b) a ## b
#define PASTE_HELPER(a, b) PASTE_HELPER_HELPER(a, b)
#define TRACE_CALL() ScopedTrace PASTE_HELPER(scopedTrace, __LINE__)(__PRETTY_FUNCTION__)
