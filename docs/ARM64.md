# isim on arm64

isim runs its guests natively: the iOS-simulator Mach-O code *is* the host's machine code, which is why a build of
isim is always one architecture. On an Intel Linux host that is x86_64, as Apple's simulator is on an Intel Mac; on an
arm64 Linux host it is arm64, as Apple's simulator is on Apple silicon. Everything that follows from that lives in one
place, [`buildlib/arch.py`](../isim/buildlib/arch.py):

| derived from the host | x86_64 host | arm64 host |
|---|---|---|
| clang target for guest code | `x86_64-apple-ios15.0-simulator` | `arm64-apple-ios15.0-simulator` |
| `ld64.lld -arch`, Mach-O CPU type | `x86_64` | `arm64` |
| `.tbd` stub target | `x86_64-ios-simulator` | `arm64-ios-simulator` |
| Swift module / interface triple | `x86_64-apple-ios-simulator` | `arm64-apple-ios-simulator` |
| `.xcframework` slice taken | `ios-x86_64-simulator` | `ios-arm64-simulator` |
| release tarball | `isim-<v>-linux-x86_64.tar.gz` | `isim-<v>-linux-aarch64.tar.gz` |
| swift.org toolchain | `ubuntu24.04` | `ubuntu24.04-aarch64` |

`isim build`, `isim cc`, `isim swiftc`, `isim test`, the Xcode-project reader (`xcodeproj.py`: `ARCHS`,
`CURRENT_ARCH`, `NATIVE_ARCH` and `arch` build-setting conditions), `install.sh` and the packaging all ask the host for
its architecture; nothing names x86_64 as *the* simulator any more.

## The two ABIs

Guest code is compiled for the guest triple above, and the runtime's host functions are compiled by the host compiler
for the host ABI. On x86-64 those are the same convention (System V), which is why the original code could hand host
functions straight to guests.

On arm64 they are AAPCS64 on both sides, with **one difference that needs bridging**: Apple passes every variadic
argument on the stack, one 8-byte slot each, in order, while AAPCS64 passes the first eight in registers. A guest's
`printf("%d %f", 1, 2.0)` therefore arrives with `sp` pointing at `1`, and libc's `printf` would read garbage.

isim bridges it without marshalling: Linux's AAPCS64 `va_list` reads the register save area first and *then that same
stack array*, so a `va_list` whose register cursors are already exhausted (`gr_offs`/`vr_offs` 0,
`runtime.h: isim_va_from_darwin`) reads Darwin's slots directly. Each guest-facing variadic function is then two
pieces, both in `runtime/`:

- a naked front end that copies `sp` into the next argument register and jumps to its body
  (`d_printf: mov x1, sp; b d_printf_v`), for the printf/scanf families, their `_l` variants, `syslog`, the
  wide-character printf family, and `open`/`openat`/`fcntl`/`ioctl`, whose optional argument is variadic too;
- the body, which rebuilds the `va_list` and calls the host's `v…` function. The `v…` entry points take a Darwin
  `va_list` (the pointer itself) and need no front end.

Other architecture-dependent pieces:

| File | What | arm64 |
|---|---|---|
| `runtime/objc_rt.c` | `objc_msgSend`, `…Super`, `…Super2`, the forwarders, `isim_objc_call_frame` | arm64 assembly (x0–x7, q0–q7, x8 for a result in memory); there is no `_stret` entry point on arm64, so those names alias the normal ones |
| `runtime/isim_internal.h` | `isim_objc_frame` / `isim_objc_call` layouts | arm64 layouts, matching the assembly's offsets |
| `runtime/objc_exc.c` | compact-unwind → CFA rules for the guest unwinder | `UNWIND_ARM64_MODE_FRAME` / `_FRAMELESS` / `_DWARF` decoding (x19–x28 pairs, d8–d15 pairs, stack size in 16-byte units) |
| `runtime/loader.c` | Mach-O checks, the unresolved-import trap page, the TLV thunk, crash reports | host CPU type, arm64 trap stub, `b isim_tlv_addr`, `hostarch.h` register access |
| `runtime/darwin_extras.inc` | `uname`, `sysctl hw.*`, the `*_l` printf variants, libunwind's `unw_*` | arm64 machine/model names, arm64 context capture, jump-free `unw_getcontext`/`unw_getreg` |
| `runtime/mach.inc` | `host_info`'s CPU type | `CPU_TYPE_ARM64` |
| `frameworks/Metal/MTLDispatch.c` | fiber context switch for CPU Metal | arm64 switch (x19–x28, fp/lr, d8–d15) |
| `frameworks/Foundation/Invocation.mrc.m` | NSInvocation argument marshalling | AAPCS64 placement: 8 integer + 8 vector registers, HFAs in v registers (CGRect is four doubles), ≤16-byte aggregates in x registers, larger results through `x8` |
| `frameworks/Foundation/Predicate.m`, `sdk-src/metal/…/__isim_metal.h` | jump buffers for the parser's and Metal's `discard()` error paths | clang has no `__builtin_setjmp`/`__builtin_longjmp` on arm64: two naked stubs save x19–x28, fp, lr and sp |
| `sdk-src/usr/include/TargetConditionals.h` | `TARGET_CPU_*` | `TARGET_CPU_ARM64 1` on arm64, as Apple's simulator reports |
| `swift/swift_float_to_string.c` | Float80 → text | arm64 has no Float80 (`long double` is `double`): the entry point forwards to the double algorithm |
| `runtime/libsystem.c` | `__extendhfsf2`, `__truncsfhf2`, `__truncdfhf2` | arm64 passes `_Float16` in a vector register on both sides, so the plain compiler-rt signatures are the ABI |

One latent x86-64 assumption turned into a real bug on arm64 and is fixed in both: `objc_rt.c` masked class data
pointers with Apple's 47-bit `FAST_DATA_MASK`, and Linux loads a PIE arm64 binary above that (`0xaab1_…`), so the mask
truncated every `class_ro_t` pointer.

## Building and testing on arm64

```bash
isim/build.py fetch && isim/build.py            # on an arm64 Linux host: arm64 simulator SDK, frameworks, apps
OS_MATRIX=1 isim/build.py test
```

CI: `.github/workflows/arm64.yml` builds and caches the same graph on GitHub's `ubuntu-24.04-arm` runners and installs
swift.org's aarch64 toolchain (`isim/ci/swift-toolchain.sh` pins its SHA-256 per architecture).

## Status

What the arm64 build has proved so far, in order: the host runtime links and runs (`--print-exports` drives the `.tbd`
generation), the SDK's `.tbd` stubs and headers generate, the Objective-C frameworks compile as arm64 Mach-O, and the
Swift support library, stdlib and overlays build. Each failure on the way was a real x86-64 assumption, and each is
fixed above rather than papered over.

Known gaps, which the x86-64 build shares unless stated:

- **`setjmp`/`longjmp` are still not in isim's SDK** — guest code that needs them uses the file-local stubs
  (`Predicate.m`, Metal's generated wrappers); adding `<setjmp.h>` to the SDK is a separate piece of work and would
  want an arm64 `jmp_buf` in the runtime.
- **Float80 does not exist on arm64** (Apple's own arm64 ABI has no extended precision type): Swift code and apps that
  use `Float80` compile only for x86_64.
- **The arm64 assembly has no runtime test of its own yet**: the entry points are assemble-checked
  (`objc_msgSend`, the varargs fronts, the TLV thunk, the fiber switch) and exercised by the framework tests, but the
  forwarding/NSInvocation paths want dedicated tests on arm64 hardware.
- **Crash-report thread state** (`.ips` register dumps) is written from the host's signal context on both
  architectures; the arm64 layout is not yet compared against a real device's report.
