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

#include "Peuck.h"

#define LOG_TAG "Peuck"

#include "Trace.h"
#include "Log.h"

#include <vector>
#include <string>
#include <map>

/************ TRACE ************/

extern "C" void BeginSection(char *name) {
  Trace *trace = Trace::getInstance();
  if (!trace->isAvailable() || !trace->isEnabled()) {
      return;
  }

  trace->beginSection(name);
}

extern "C" void EndSection() {
  Trace *trace = Trace::getInstance();
  trace->endSection();
}

/************ Cpu Affintiy ************/

#include <sys/syscall.h>
#include <pthread.h>
#include <unistd.h>

bool startsWith(std::string mainStr, std::string toMatch) {
	// std::string::find returns 0 if toMatch is found at starting
	if(mainStr.find(toMatch) == 0)
		return true;
	else
		return false;
}

void eraseSubStr(std::string & mainStr, const std::string & toErase) {
	// Search for the substring in string
	size_t pos = mainStr.find(toErase);

	if (pos != std::string::npos) {
		// If found then erase it from string
		mainStr.erase(pos, toErase.length());
	}
}

std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> v;
    std::string::size_type i = 0;
    std::string::size_type j = s.find(c);

    while (j != std::string::npos) {
      v.push_back(s.substr(i, j-i));
      i = ++j;
      j = s.find(c, j);

      if (j == std::string::npos)
         v.push_back(s.substr(i, s.length()));
   }
   return v;
}

struct Cpu {
  struct Core {
    int id;
    int frequency;
    int package_id;
  };

  std::vector<Core> cores;
  std::string hardware;
  std::map<int, std::string> threads;
} cpu;

std::string ReadFile(const std::string& path) {
  char buf[10240];
  FILE *fp = fopen(path.c_str(), "r");
  if (fp == nullptr)
    return std::string();

  fgets(buf, 10240, fp);
  fclose(fp);
  return std::string(buf);
}

void ReadCpuInfo() {
  char buf[10240];
  FILE *fp = fopen("/proc/cpuinfo", "r");

  if (fp) {
    while (fgets(buf, 10240, fp) != NULL) {
      buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
      std::string line = buf;

      if (startsWith(line, "processor")) {
        auto frequency = ReadFile(std::string("/sys/devices/system/cpu/cpu") + std::to_string(cpu.cores.size()) + "/cpufreq/cpuinfo_max_freq");
        auto package_id = ReadFile(std::string("/sys/devices/system/cpu/cpu") + std::to_string(cpu.cores.size()) + "/topology/physical_package_id");

        Cpu::Core core;
        core.id = cpu.cores.size();
        core.frequency = std::atoi(frequency.c_str());
        core.package_id = std::atoi(package_id.c_str());

        cpu.cores.push_back(core);
      }
      if (startsWith(line, "Hardware")) {
        cpu.hardware = split(line, ':')[1];
      }

      //  /sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_max_freq

      // ALOGE("[%d]: %s", cpu.cores, line.c_str());
    }
    fclose(fp);
  }
  ALOGE("CPU cores = %d", (int) cpu.cores.size());
  ALOGE("CPU hardware = %s", cpu.hardware.c_str());
}

#include <dirent.h>

void FindAllThreads() {
  struct dirent *entry;
  DIR *dir = opendir("/proc/self/task/");
  if (dir == NULL) {
      return;
  }

  char buf[10240];

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    int tid = std::atoi(entry->d_name);
    std::string task_path = std::string("/proc/self/task/") + std::string(entry->d_name) + "/stat";
    FILE *fp = fopen(task_path.c_str(), "r");
    fgets(buf, 10240, fp);
    fclose(fp);

    auto thread_name = split(std::string(buf), '(')[1];

    thread_name = split(thread_name, ')')[0];
    // ALOGE("%d %s", tid, thread_name.c_str());

    cpu.threads.insert(std::make_pair(tid, thread_name));
  }

  closedir(dir);
}

extern "C" const char* EnumerateThreads() {
  FindAllThreads();

  std::string threads;
  for (const auto& thread : cpu.threads) {
    threads += std::string(std::to_string(thread.first)) + " " + thread.second + "\n";
  }

  return threads.c_str();
}

extern "C" int GetCoresCount() {
  // not thread safe
  if (cpu.cores.empty()) {
    ReadCpuInfo();
  }

  return cpu.cores.size();
}

extern "C" const char* GetCpuTopology() {
  if (cpu.cores.empty()) {
    ReadCpuInfo();
  }

  return "";
}

extern "C" const char* GetCpuHardware() {
  // not thread safe
  if (cpu.cores.empty()) {
    ReadCpuInfo();
  }

  return cpu.hardware.c_str();
}

extern "C" void SetThreadAffinityMask(int tid, int mask) {
  int err, syscallres;
  syscallres = syscall(__NR_sched_setaffinity, tid, sizeof(mask), &mask);
  ALOGE("****** Set affinity mask for tid: %d 0x%x", tid, mask);

  if (syscallres) {
      err = errno;
      ALOGE("Error in the syscall setaffinity: mask=%d=0x%x err=%d=0x%x", mask, mask, err, err);
  }
}

extern "C" void SetThreadAffinityMaskByName(char *name, int mask) {
  if (cpu.threads.size() == 0) {
    FindAllThreads();
  }

  std::string thread_name(name);

  for (const auto& thread : cpu.threads) {
    if (thread.second == thread_name) {
      ALOGE("Found thread %s:%d", thread_name.c_str(), thread.first);
      SetThreadAffinityMask(thread.first, mask);
    }
  }
}

extern "C" void SetCurrentThreadAffinityMask(int mask) {
    SetThreadAffinityMask(gettid(), mask);
}
