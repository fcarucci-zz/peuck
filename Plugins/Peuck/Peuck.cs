using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System;
using System.Runtime.InteropServices;

namespace Peuck
{
    public class Trace
    {
        public static void BeginSection(string name) 
        {
#if !UNITY_ANDROID
            return;
#endif
            Internal.Trace.BeginSection(name);
        }

        public static void EndSection()
        {
#if !UNITY_ANDROID
            return;
#endif
            Internal.Trace.EndSection();
        }
    }

    public class CPU
    {
        public static int GetCpuCount()
        {
#if !UNITY_ANDROID
            return 0;
#endif

            return Internal.CPU.GetCpuCount();
        }

        public static string GetCpuHardware()
        {
#if !UNITY_ANDROID
            return "";
#endif

            return Internal.CPU.GetCpuHardware();
        }

        public static Dictionary<int, string> EnumerateThreads()
        {
            Dictionary<int, string> threads = new Dictionary<int, string>();

#if UNITY_ANDROID
            string threads_string = Internal.CPU.EnumerateThreads();

#else
            string threads_string = "";

#endif

            string[] lines = threads_string.Split(new string[] { "\n" }, StringSplitOptions.None);


            foreach (string line in lines) 
            {
                Debug.Log(line);
                string[] tokens = line.Split(new string[] { " " }, StringSplitOptions.None);
                if (tokens.Length < 2)
                    continue;
                int tid;
                Int32.TryParse(tokens[0], out tid);
                threads.Add(tid, tokens[1]);
            }

            return threads;
        }

        public static void SetCurrentThreadAffinityMask(int mask)
        {
#if !UNITY_ANDROID
            return;
#endif
            Internal.CPU.SetCurrentThreadAffinityMask(mask);
        }

        public static void SetThreadAffinityMaskByName(string name, int mask)
        {
#if !UNITY_ANDROID
            return;
#endif
            Internal.CPU.SetThreadAffinityMaskByName(name, mask);
        }

        public static void SetThreadAffinityMask(int tid, int mask)
        {
#if !UNITY_ANDROID
            return;
#endif
            Internal.CPU.SetThreadAffinityMask(tid, mask);
        }
    }

    namespace Internal
    {
        public class Trace
        {
            [DllImport("peuck")]
            public static extern void BeginSection(string name);

            [DllImport("peuck")]
            public static extern void EndSection();
        }

        public class CPU
        {
            [DllImport("peuck")]
            public static extern int GetCpuCount();

            [DllImport("peuck")]
            public static extern string GetCpuHardware();

            [DllImport("peuck")]
            public static extern string EnumerateThreads();

            [DllImport("peuck")]
            public static extern void SetCurrentThreadAffinityMask(int mask);

            [DllImport("peuck")]
            public static extern void SetThreadAffinityMaskByName(string name, int mask);

            [DllImport("peuck")]
            public static extern void SetThreadAffinityMask(int tid, int mask);
        }
    }
}

