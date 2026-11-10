"""The architectures isim builds for.

isim executes its guests natively: the iOS-simulator Mach-O code is the host's own machine code, so the guest
architecture is the host's, not a choice. On an x86-64 Linux host every guest binary is x86_64 (as in Apple's
simulator on an Intel Mac); on an arm64 host every guest binary is arm64 (as in Apple's simulator on Apple silicon).

Everything arch-dependent in the build graph, the tools and the release naming comes from here, so a port adds a
mapping and nothing else:

  * ``HOST``   the machine the release runs on: ``x86_64`` or ``arm64`` (``uname -m`` folded to these two names).
  * ``GUEST``  the Mach-O architecture of the SDK, the frameworks, the built apps and the ABI the runtime implements:
               ``x86_64`` or ``arm64``.

``ISIM_HOST_ARCH`` overrides the detection (a cross-check, or a build named for one architecture).
"""
import os
import platform

_HOST_NAMES = {"x86_64": "x86_64", "amd64": "x86_64", "x64": "x86_64",
               "aarch64": "arm64", "arm64": "arm64", "armv8l": "arm64"}


def _detect():
    name = (os.environ.get("ISIM_HOST_ARCH") or platform.machine() or "").lower()
    host = _HOST_NAMES.get(name)
    if host is None:
        raise SystemExit(f"isim: unsupported host architecture '{platform.machine()}': isim builds for x86_64 (amd64) "
                         f"and arm64 (aarch64) Linux")
    return host


HOST = _detect()
GUEST = HOST                       # native execution: guest code is the host's machine code


def triple(minos="15.0"):
    """clang's target for guest code: 'x86_64-apple-ios15.0-simulator' or 'arm64-apple-ios15.0-simulator'"""
    return f"{GUEST}-apple-ios{minos}-simulator"


def platform_triple():
    """the target name in .tbd stubs and Swift module directories: 'x86_64-ios-simulator' or 'arm64-ios-simulator'"""
    return f"{GUEST}-ios-simulator"


def module_triple():
    """the Swift module directory's triple suffix: 'x86_64-apple-ios-simulator' (no version)"""
    return f"{GUEST}-apple-ios-simulator"


def tarball_arch():
    """a release tarball's architecture: 'linux-x86_64' or 'linux-aarch64'"""
    return "linux-" + ("x86_64" if HOST == "x86_64" else "aarch64")
