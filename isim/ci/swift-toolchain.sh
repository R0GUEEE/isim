#!/usr/bin/env bash
# Swift 6.2.4 from swift.org (the toolchain isim's SDK is built with, the same one swiftly installs), checked
# against its pinned SHA-256 and unpacked into DIR (DIR/usr/bin/swiftc), for CI and cloud sessions:
# ISIM_SWIFT_TOOLCHAIN=DIR then compiles with it. On a desktop, `isim toolchain install` (swiftly) installs the same
# toolchain. Usage: isim/ci/swift-toolchain.sh DIR
set -euo pipefail
dir=${1:?usage: swift-toolchain.sh DIR}
version=6.2.4
# the tarball per host architecture. the SHA-256s are pinned, so CI does not depend on fetching swift.org's signing
# keys; each is the checksum of the tarball for that architecture at that release (the same tarball
# `isim toolchain install` verifies with "Swift 6.x Release Signing Key", 52BB 7E3D E28A 71BE 22EC 05FF EF80 A866
# B47A 981F, on a desktop)
case "$(uname -m)" in
  x86_64|amd64)  platform=ubuntu24.04;         sha256=15608c4fa0364ef906014343d81639cf58169e8a40de2b2d3503c3f35a8bb66d ;;
  aarch64|arm64) platform=ubuntu24.04-aarch64; sha256=420bcde2ee4b2a36e49524b15373d0cb24eb4b679103f8cc8349af8a768832b7 ;;
  *) echo "swift-toolchain.sh: no swift.org build for $(uname -m)" >&2; exit 1 ;;
esac
name=swift-$version-RELEASE-$platform
url=https://download.swift.org/swift-$version-release/${platform//./}/swift-$version-RELEASE/$name.tar.gz
if [ -x "$dir/usr/bin/swift-frontend" ] && "$dir/usr/bin/swift-frontend" --version | grep -q "(swift-$version-RELEASE)"; then
  echo "Swift $version is in $dir"; exit 0
fi
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
curl -fsSL --retry 3 -o "$tmp/swift.tar.gz" "$url"
echo "$sha256  $tmp/swift.tar.gz" | sha256sum -c -
mkdir -p "$dir"
tar -xzf "$tmp/swift.tar.gz" -C "$dir" --strip-components=1
"$dir/usr/bin/swift-frontend" --version | head -1
