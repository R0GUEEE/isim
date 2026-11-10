/* isim runtime: the host CPU's own pieces the runtime reads — the signal context's program counter, frame pointer and
 * stack pointer, the names Apple's crash reports use, and one x86-64 detail of the call/return addresses.
 *
 * Guest code is the host's machine code (native execution), so "the host architecture" and "the guest architecture"
 * are the same: x86_64 on an Intel Linux host, arm64 on an AArch64 one. Everything that names a register or a CPU type
 * goes through here (and the very few hand-written entry points in objc_rt.c / loader.c have one block per
 * architecture).
 */
#pragma once
#include <stdint.h>
#include <ucontext.h>

#if defined(__x86_64__)
#define ISIM_ARCH_NAME "x86_64"
#define ISIM_CPU_TYPE_NAME "X86-64"             /* the "cpuType" of an Apple .ips crash report */
/* the interrupted thread's state: x86-64 keeps RIP/RBP/RSP in gregs[] */
#define ISIM_UC_PC(uc) ((uintptr_t)(uc)->uc_mcontext.gregs[REG_RIP])
#define ISIM_UC_FP(uc) ((uintptr_t)(uc)->uc_mcontext.gregs[REG_RBP])
#define ISIM_UC_SP(uc) ((uintptr_t)(uc)->uc_mcontext.gregs[REG_RSP])
/* a call's return address is the instruction after the call (the unwinder's IP is one past a call site) */
#define ISIM_IP_AT_CALL_SITE(pc) ((pc) + 1)
#elif defined(__aarch64__)
#define ISIM_ARCH_NAME "arm64"
#define ISIM_CPU_TYPE_NAME "ARM-64"
/* AArch64: pc, sp and regs[31] (x29 is the frame pointer, x30 the link register) */
#define ISIM_UC_PC(uc) ((uintptr_t)(uc)->uc_mcontext.pc)
#define ISIM_UC_FP(uc) ((uintptr_t)(uc)->uc_mcontext.regs[29])
#define ISIM_UC_SP(uc) ((uintptr_t)(uc)->uc_mcontext.sp)
/* a call's return address is the link register of that call: it already points at the caller's next instruction */
#define ISIM_IP_AT_CALL_SITE(pc) (pc)
#else
#error "isim runs on x86-64 and arm64 Linux: add this host architecture's signal context here"
#endif
