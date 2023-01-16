/*
 *  system.c
 *  artoolkitX
 *
 *  Functions to query various system-related parameters.
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015-2016 Daqri, LLC.
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <ARX/ARUtil/system.h>

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#include <stdio.h>
#include <string.h> // strdup/_strdup
#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__APPLE__)
#  include <TargetConditionals.h>
#  include <sys/types.h>
#  include <sys/sysctl.h> // sysctlbyname()
#  include <mach/machine.h> // cpu_type_t, cpu_subtype_t
#  if TARGET_OS_IPHONE
#    ifndef __OBJC__
#      error system.c must be compiled as Objective C.
#    endif
#    import <UIKit/UIDevice.h> // UIDevice
#  else
#    import <CoreServices/CoreServices.h> // Gestalt()
#  endif
#elif defined(ANDROID)
#  include <ARX/ARUtil/android.h>
#elif defined(__linux)
#  include <sys/utsname.h> // uname()
#endif

#ifdef _WIN32
#  include <libloaderapi.h>
#else
#  include <dlfcn.h> // dladdr
#endif

char *arUtilGetOSName(void)
{
#if defined(__APPLE__) // iOS and OS X.
#  if TARGET_OS_IPHONE
    return strdup("ios");
#  else
    return strdup("macos");
#  endif
#elif defined(ANDROID) // Android
    return strdup("android");
#elif defined(_WIN32) // Windows.
    return _strdup("windows");
#elif defined(__linux) // Linux.
    return strdup("linux");
#else // Other.
    return strdup("unknown");
#endif
}

char *arUtilGetOSVersion(void)
{
    char *ret = NULL;
#if defined(__APPLE__) // iOS and OS X.
#  if TARGET_OS_IPHONE
    ret = strdup([[[UIDevice currentDevice] systemVersion] UTF8String]);
#  else
    size_t size;;
    sysctlbyname("kern.osproductversion", NULL, &size, NULL, 0);
    ret = malloc(size);
    sysctlbyname("kern.osproductversion", ret, &size, NULL, 0);
#  endif
#elif defined(ANDROID) // Android
    char os[PROP_VALUE_MAX];
    android_system_property_get(ANDROID_OS_BUILD_VERSION_RELEASE, os);
    ret = strdup(os);
#elif defined(_WIN32) // Windows.
    ret = _strdup("unknown");
#elif defined(__linux) // Linux.
    struct utsname un;
    if (uname(&un) == 0) {
        ret = strdup(un.release);
    } else {
        ret = strdup("unknown");
    }
#else // Other.
    ret = strdup("unknown");
#endif
    return (ret);
}

char *arUtilGetCPUName(void)
{
    char *ret = NULL;
#if defined(__APPLE__)
    static const char *cpuTypeNameUnknown = "unknown";
    static const char *cpuTypeNameARMv6 = "armv6";
    static const char *cpuTypeNameARMv7 = "armv7";
    static const char *cpuTypeNameARMv7s = "armv7s";
    static const char *cpuTypeNameARMv7k = "armv7k"; // Apple Watch.
    static const char *cpuTypeNameARM64 = "armv64";
    static const char *cpuTypeNameX86 = "x86";
    static const char *cpuTypeNameX86_64 = "x86_64";

    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);
    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);

    // Values for cputype and cpusubtype defined in <mach/machine.h>.
    const char *namep = cpuTypeNameUnknown;
    if (type == CPU_TYPE_ARM) {
        switch (subtype) {
            case CPU_SUBTYPE_ARM_V7S:
                namep = cpuTypeNameARMv7s;
                break;
            case CPU_SUBTYPE_ARM_V7:
                namep = cpuTypeNameARMv7;
                break;
            case CPU_SUBTYPE_ARM_V6:
                namep = cpuTypeNameARMv6;
                break;
            case CPU_SUBTYPE_ARM_V7K:
                namep = cpuTypeNameARMv7k;
                break;
        }
    } else if (type == CPU_TYPE_ARM64) {
        namep = cpuTypeNameARM64;
    } else if (type == CPU_TYPE_X86) {
        switch (subtype) {
            case CPU_SUBTYPE_X86_64_H:
                namep = cpuTypeNameX86_64;
                break;
            default:
                namep = cpuTypeNameX86;
                break;
        }
    } else if (type == CPU_TYPE_X86_64) {
        namep = cpuTypeNameX86_64;
    }
    ret = strdup(namep);
#elif defined(ANDROID)
    char os[PROP_VALUE_MAX];
    android_system_property_get(ANDROID_OS_BUILD_CPU_ABI, os);
    ret = strdup(os);
#elif defined(_WIN32) // Windows.
#  if defined(_M_IX86)
    ret = _strdup("x86");
#  elif defined(_M_X64)
    ret = _strdup("x86_64");
#  elif defined(_M_IA64)
    ret = _strdup("ia64");
#  elif defined(_M_ARM)
    ret = _strdup("arm");
#  else
    ret = _strdup("unknown");
#  endif
#elif defined(__linux) // Linux.
    struct utsname un;
    if (uname(&un) == 0) {
        ret = strdup(un.machine);
    } else {
        ret = strdup("unknown");
    }
#else
    ret = strdup("unknown");
#endif
    return (ret);
}

char *arUtilGetModulePath(void)
{
#ifdef _WIN32
    char path[MAX_PATH];
    HMODULE hm = NULL;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)arUtilGetModulePath, &hm) == 0) {
        //int ret = GetLastError();
        //fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
        return NULL;
    }
    if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
        //int ret = GetLastError();
        //fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
        return NULL;
    }
    return (_strdup(path));
#else
    Dl_info info;
    if (dladdr(arUtilGetModulePath, &info) == 0) {
        return NULL;
    }
    return (strdup(info.dli_fname));
#endif
}
