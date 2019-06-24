using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System;
using System.Runtime.InteropServices;

// Unity Thread affinity
//
// Example:
// CPU.SetUnityThreadAffinity(CPU.UnityThreads.Main, CPU.Cores.Little);
//
// Unity threads:
//  CPU.UnityThreads.Main
//  CPU.UnityThreads.GFX
//  CPU.UnityThreads.Choreographer
//  CPU.UnityThreads.WorkerThread
//  CPU.UnityThreads.FMODMixer
//
// Only works on 8 core SoCs for now (eg. Qualcomm SDM845, SDM835

namespace Peuck
{
    //public static void SetUnityThreadAffinity()
    public class Trace
    {
        public static void BeginSection(string name)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            Internal.Trace.BeginSection(name);
#endif
        }

        public static void EndSection()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            Internal.Trace.EndSection();
#endif
        }
    }

    public class CPU
    {
        public struct Core
        {
            public int index;
            public long frequency;
            public int cluster;
        }

        public enum Cores { All = 0, Little = 1, Big = 2 }

        public enum UnityThreads { Main = 1, GFX, Choreographer, WorkerThread, BackgroundWorker, FMODMixer, FMODStreamer };

        private static Dictionary<UnityThreads, string> unityThreadsMap = new Dictionary<UnityThreads, string>()
            {
                { UnityThreads.Main, "UnityMain" },
                { UnityThreads.GFX, "UnityGfxDeviceW" },
                { UnityThreads.Choreographer, "UnityChoreograp" },
                { UnityThreads.WorkerThread, "Worker Thread" },
                { UnityThreads.FMODMixer, "FMOD mixer thre" },
                { UnityThreads.FMODStreamer, "FMOD stream thr" },
                { UnityThreads.BackgroundWorker, "BackgroundWorke" },
            };

        public static void SetUnityThreadAffinity(UnityThreads thread, Cores cores)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            // Only support 8 core SoC for now, do nothing otherwise
            if (GetCoresCount() != 8)
            {
                return;
            }

            int mask = 0xFFFF;

            // Assume for now that the first 4 cores are little cores in a 8 core SoC.
            // TODO: Use CPU topology to compute the mask
            if (cores == Cores.Little)
            {
                mask = 0x0F;
            }
            if (cores == Cores.Big)
            {
                mask = 0xF0;
            }

            SetThreadAffinityMaskByName(unityThreadsMap[thread], mask);
#endif
        }

        public static int GetCoresCount()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            return Internal.CPU.GetCoresCount();
#else
            return 0;
#endif
        }

        public static string GetCpuHardware()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            return Internal.CPU.GetCpuHardware();
#else
            return null;
#endif
        }

        public static List<Core> GetCpuTopology()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            string topology_string = Internal.CPU.GetCpuTopology();
#else
            string topology_string = null;
#endif
            List<Core> topology = new List<Core>();

            if (topology_string == null)
                return topology;

            string[] lines = topology_string.Split('\n');

            foreach (var line in lines)
            {
                string[] tokens = line.Split(' ');

                if (tokens.Length < 3)
                    continue;

                Core core = new Core();

                core.index = Convert.ToInt32(tokens[0]);
                core.frequency = Convert.ToInt64(tokens[1]);
                core.cluster = Convert.ToInt32(tokens[2]);

                topology.Add(core);
            }

            return topology;
        }

        public static Dictionary<int, string> EnumerateThreads()
        {
            Dictionary<int, string> threads = new Dictionary<int, string>();

#if UNITY_ANDROID && !UNITY_EDITOR
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

                threads.Add(tid, string.Join(" ", tokens, 1, tokens.Length -1));
            }

            return threads;
        }

        public static void SetCurrentThreadAffinityMask(int mask)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            Internal.CPU.SetCurrentThreadAffinityMask(mask);
#endif
        }

        public static void SetThreadAffinityMaskByName(string name, int mask)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            Internal.CPU.SetThreadAffinityMaskByName(name, mask);
#endif
        }

        public static void SetThreadAffinityMask(int tid, int mask)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            Internal.CPU.SetThreadAffinityMask(tid, mask);
#endif
        }
    }

#if UNITY_ANDROID && !UNITY_EDITOR
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
            public static extern int GetCoresCount();

            [DllImport("peuck")]
            public static extern string GetCpuHardware();

            [DllImport("peuck")]
            public static extern string GetCpuTopology();

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
#endif
}
