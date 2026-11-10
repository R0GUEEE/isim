/* isim runtime: internal interfaces between loader, host libraries and ObjC runtime. */
#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "guest_abi.h"           /* ISIM_GUEST_ABI, ISIM_GUEST_CALLBACK */
#include "hostarch.h"            /* the host CPU: signal context fields, crash-report names */

/* Status labels for every host-provided symbol (see docs/COVERAGE.md):
 *   "passthrough" host glibc function; ABI-identical for its signature on the host (x86_64: SysV, arm64: AAPCS64
 *                 with Darwin's variadic rule bridged in libsystem.c; long double is the one type that differs)
 *   "adapted"     wrapper translating Darwin layout/semantics/constants
 *   "isim"        isim-specific implementation (ObjC runtime, host graphics bridge)
 *   "stub"        placeholder that does NOT implement Darwin behaviour */
#if defined(__aarch64__)
/* The Darwin variadic bridge (arm64 only: x86-64 Darwin and Linux agree on va_list).
 *
 * Apple's arm64 ABI passes every variadic argument on the stack, one 8-byte slot each and in order — nothing in the
 * registers — so at the entry of a variadic function sp points at the first of them. Linux's AAPCS64 va_list reads
 * the register save area first and then that same stack array; a va_list whose register cursors are already exhausted
 * (gr_offs and vr_offs 0) therefore reads Darwin's slots directly, in order, with no marshalling at all.
 *
 * A guest-facing variadic host function is then two pieces: a naked front end that copies sp into the next argument
 * register and jumps to a C body (the "bodies" at the bottom of libsystem.c, libc_extras.inc and darwin_extras.inc),
 * and the body, which rebuilds the va_list with this and calls the host's v-function. */
/* a C body only hand-written assembly calls (the bridge's front ends): emitted even though no C code names it */
#define ISIM_ASM_CALLEE __attribute__((used))
typedef struct { void *stack, *gr_top, *vr_top; int gr_offs, vr_offs; } isim_aarch64_va_list;
static inline void isim_va_from_darwin(va_list *ap, const void *stack) {
    isim_aarch64_va_list v = { (void *)stack, (void *)stack, (void *)stack, 0, 0 };
    _Static_assert(sizeof v == sizeof(va_list), "aarch64 va_list: five fields, 32 bytes");
    memcpy(ap, &v, sizeof v);
}
#endif

struct shim { const char *name; void *addr; const char *status; };

/* host pseudo-libraries: install name -> symbol table */
struct host_lib { const char *install_name; const struct shim *table; size_t count; void (*load)(void); /* before the first lookup */ };
extern const struct host_lib host_libsystem, host_libobjc, host_isim, host_sqlite, host_libz, host_libcompression, host_jsc;
extern const struct host_lib host_commoncrypto, host_libsystem_os;   /* searched as part of libSystem */
void host_sqlite_load(void);                       /* dlopens the host libsqlite3 (before the first lookup) */
const struct shim *host_lib_lookup(const struct host_lib *lib, const char *name);

/* loader services */
void *isim_lookup_symbol(const char *mangled_name); /* flat lookup over images + host libs */
const char *isim_main_executable_path(void);
const char *isim_container_home(void);          /* host_os.c: the app container, "" until Foundation says */
const char *isim_image_path_for_address(const void *addr);
void isim_fatal(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
/* stacks and symbols (loader.c): a thread's stack sampled from a signal; a pc as its image, symbol and offsets */
ISIM_GUEST_ABI int isim_sample_thread(int tid, uintptr_t *pcs, int max);
int isim_symbolize(uintptr_t pc, char *image, size_t icap, char *sym, size_t scap, uint64_t *image_offset,
                   uint64_t *sym_offset, uint64_t *image_base);

/* diagnostics (host_os.c, host_diag.c) */
ISIM_GUEST_ABI void isim_log_record(int kind, int type, const char *subsystem, const char *category, const char *message,
                     const char *format, int sptype, uint64_t spid, const char *name);
ISIM_GUEST_ABI void isim_metric_add(const char *key, double v);
ISIM_GUEST_ABI void isim_metric_max(const char *key, double v);
ISIM_GUEST_ABI double isim_metric_get(const char *key);
ISIM_GUEST_ABI const char *isim_diag_app(int which);              /* 0 bundle id, 1 version, 2 build, 3 name, 4 bundle path */
ISIM_GUEST_ABI double isim_diag_process_start(void);
void isim_diag_frame(void);
void isim_diag_beat(int waiting);
extern int isim_verbose;

/* ObjC runtime hooks (objc_rt.c) */
struct objc_image {
    const char *name;
    void *(*find_section)(const struct objc_image *img, const char *sectname, uint64_t *size);
    void *ctx;
};
void objc_rt_map_image(const struct objc_image *img);   /* after fixups, before mprotect */
void objc_rt_load_image(const struct objc_image *img);  /* +load methods, after all images mapped */
void objc_rt_audit(void);
struct objc_class *objc_rt_immortal_class(const char *name);   /* a root class whose objects are never freed (os_log_t) */                                /* ISIM_OBJC_AUDIT=1: unimplemented selectors, before main */

void libsystem_init(int argc, char **argv);
