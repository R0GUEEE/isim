#pragma once
/* isim runtime private entry points used by the isim Foundation implementation. */
#include <objc/runtime.h>
__BEGIN_DECLS
id _Nullable _objc_rootRetain(id _Nullable);
void _objc_rootRelease(id _Nullable);
BOOL _objc_rootReleaseWasZero(id _Nonnull);
unsigned long _objc_rootRetainCount(id _Nonnull);
id _Nullable _objc_rootAutorelease(id _Nullable);
BOOL _objc_rootIsDeallocating(id _Nonnull);
void * _Nonnull objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void * _Nonnull);
__attribute__((noreturn)) void objc_exception_throw(id _Nonnull);

/* message forwarding: _objc_msgForward(_stret) saves the argument registers of the unhandled send in
 * an isim_objc_frame and calls the handler (Foundation's NSInvocation-based forwarding). The handler
 * returns a new receiver to re-send the message to (forwardingTargetForSelector:), or nil after it
 * stored the return value in ret_gpr/ret_xmm (or in the stret buffer, gpr[0]). */
#if defined(__arm64__) || defined(__aarch64__)
/* arm64 (Apple's ABI): 8 integer argument registers x0-x7 and 8 vector registers q0-q7, x8 for a result returned
 * through memory, further arguments on the stack. The offsets are the ones objc_rt.c's assembly saves at (change one,
 * change the other): gpr 0x00, vr 0x40, stack 0xc0, x8 0xc8, stret 0xd0, ret_gpr 0xd8, ret_vr 0x100. */
typedef struct isim_objc_frame {
    uint64_t gpr[8];             /* x0-x7: x0 = receiver, x1 = _cmd */
    uint64_t vr[8][2];           /* q0-q7 */
    uint64_t * _Nonnull stack;   /* first stack-passed argument */
    uint64_t x8;                 /* the hidden result pointer of a struct returned through memory */
    uint64_t stret;              /* 1: entered through a *_stret entry point (arm64 has none: always 0) */
    uint64_t ret_gpr[4];         /* x0-x3 when returning */
    uint64_t ret_pad;            /* keeps ret_vr 16-byte aligned */
    uint64_t ret_vr[4][2];       /* q0-q3 when returning */
} isim_objc_frame;
typedef id _Nullable (*isim_objc_forward_handler)(id _Nonnull self, SEL _Nonnull sel, isim_objc_frame * _Nonnull frame);
void isim_objc_set_forward_handler(isim_objc_forward_handler _Nullable handler);
/* calls fn with the given argument registers (arm64 AAPCS64) and nstack words of stack arguments */
typedef struct isim_objc_call {
    void * _Nonnull fn;          /* 0x00 */
    uint64_t gpr[8];             /* 0x08 x0-x7 */
    uint64_t vr_pad;             /* 0x48 */
    uint64_t vr[8][2];           /* 0x50 q0-q7 */
    const uint64_t * _Nullable stack;   /* 0xd0 */
    uint64_t nstack, nvec;       /* 0xd8, 0xe0 */
    uint64_t x8;                 /* 0xe8 the hidden result pointer of a struct returned through memory */
    uint64_t ret_gpr[4];         /* 0xf0 x0-x3 */
    uint64_t ret_vr[4][2];       /* 0x110 q0-q3 */
} isim_objc_call;
void isim_objc_call_frame(isim_objc_call * _Nonnull call);
#else
typedef struct isim_objc_frame {
    uint64_t gpr[6];             /* rdi rsi rdx rcx r8 r9 */
    uint64_t xmm[8][2];          /* xmm0-xmm7 */
    uint64_t * _Nonnull stack;   /* first stack-passed argument */
    uint64_t rax;                /* vector register count of variadic calls */
    uint64_t ret_gpr[2];         /* rax, rdx when returning */
    uint64_t ret_xmm[2][2];      /* xmm0, xmm1 when returning */
    uint64_t stret;              /* 1: entered through _objc_msgForward_stret (gpr[0] = result buffer) */
    uint64_t reserved;
} isim_objc_frame;
typedef id _Nullable (*isim_objc_forward_handler)(id _Nonnull self, SEL _Nonnull sel, isim_objc_frame * _Nonnull frame);
void isim_objc_set_forward_handler(isim_objc_forward_handler _Nullable handler);
/* calls fn with the given argument registers (x86_64 SysV) and nstack words of stack arguments */
typedef struct isim_objc_call {
    void * _Nonnull fn;
    uint64_t gpr[6];
    uint64_t xmm[8][2];
    const uint64_t * _Nullable stack;
    uint64_t nstack, nvec;
    uint64_t ret_gpr[2];
    uint64_t ret_xmm[2][2];
} isim_objc_call;
void isim_objc_call_frame(isim_objc_call * _Nonnull call);
#endif
__END_DECLS
