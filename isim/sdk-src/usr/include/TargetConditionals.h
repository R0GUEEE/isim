#pragma once
/* isim SDK: the only supported target is the iOS simulator, on the host's architecture — x86_64 on an Intel Linux
 * host, arm64 on an AArch64 one (the guest code is the host's machine code), as Apple's simulator reports for its Mac. */
#define TARGET_OS_MAC 1
#define TARGET_OS_IPHONE 1
#define TARGET_OS_IOS 1
#define TARGET_OS_OSX 0
#define TARGET_OS_TV 0
#define TARGET_OS_WATCH 0
#define TARGET_OS_VISION 0
#define TARGET_OS_MACCATALYST 0
#define TARGET_OS_SIMULATOR 1
#define TARGET_IPHONE_SIMULATOR 1
#if defined(__aarch64__)
#define TARGET_CPU_ARM64 1
#define TARGET_CPU_X86_64 0
#else
#define TARGET_CPU_X86_64 1
#define TARGET_CPU_ARM64 0
#endif
#define TARGET_RT_64_BIT 1
#define TARGET_OS_ISIM 1
