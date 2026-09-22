#!/usr/bin/env bash
# Build the local VKB workaround using the reader's own initialized Wine prefix.
set -euo pipefail
if (( $# < 1 || $# > 2 )); then
    printf 'Usage: bash build.sh /absolute/path/to/wineprefix [output-directory]\n' >&2
    exit 2
fi
VKB_SOURCE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VKB_PREFIX="$1"
VKB_OUTPUT="${2:-$VKB_SOURCE_DIR/build}"
command -v i686-w64-mingw32-gcc >/dev/null || { printf 'Missing i686-w64-mingw32-gcc (32-bit MinGW-w64 compiler).\n' >&2; exit 1; }
command -v python3 >/dev/null || { printf 'Missing python3.\n' >&2; exit 1; }
# Validate the original library before compiling or installing anything.
python3 - "$VKB_PREFIX" "$VKB_OUTPUT" <<'PY'
from pathlib import Path
import hashlib, json, struct, sys
prefix = Path(sys.argv[1]).expanduser().resolve()
out = Path(sys.argv[2]).expanduser().resolve()
if not (prefix / 'system.reg').is_file():
    raise SystemExit('Not an initialized Wine prefix. Run wineboot -u with that WINEPREFIX first.')
chosen = None
for relative in ['drive_c/windows/syswow64/dinput8.dll', 'drive_c/windows/system32/dinput8.dll']:
    candidate = prefix / relative
    if not candidate.is_file():
        continue
    data = candidate.read_bytes()
    if len(data) < 128 or data[:2] != b'MZ':
        continue
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if offset + 6 > len(data) or data[offset:offset+4] != b'PE\0\0':
        continue
    if struct.unpack_from('<H', data, offset+4)[0] != 0x14c:
        continue
    if data[64:80] != b'Wine builtin DLL':
        raise SystemExit(f'{candidate} is not the expected Wine builtin DLL. Use a dedicated prefix without a native dinput8 override.')
    chosen = candidate
    break
if chosen is None:
    raise SystemExit('No i386 Wine dinput8.dll found in this prefix. This workaround requires the 32-bit PE Wine library.')
out.mkdir(parents=True, exist_ok=True)
copy = bytearray(data)
# Only the private copy's DOS stub changes. PE executable code is untouched.
copy[64:80] = b'VKB private DLL!'
(out / 'vkb-wine-dinput8.dll').write_bytes(copy)
(out / 'build-info.json').write_text(json.dumps({
    'wine_library_source': str(chosen),
    'original_library_sha256': hashlib.sha256(data).hexdigest(),
    'private_library_sha256': hashlib.sha256(copy).hexdigest(),
    'architecture': 'i386',
    'change': 'DOS-stub marker bytes 64..79 only'
}, indent=2) + '\n')
print('Prepared private 32-bit Wine library copy.')
PY
i686-w64-mingw32-gcc -shared -O2 -Wall -Wextra \
    -Wno-unused-parameter -Wno-cast-function-type -Wl,--kill-at \
    -o "$VKB_OUTPUT/dinput8.dll" \
    "$VKB_SOURCE_DIR/src/dinput8-vkb.c" "$VKB_SOURCE_DIR/src/dinput8.def"
printf 'Built: %s/dinput8.dll\nBuilt: %s/vkb-wine-dinput8.dll\n' "$VKB_OUTPUT" "$VKB_OUTPUT"
printf 'Follow docs/BUILD.md to install both files beside VKBDevCfg-C.exe.\n'
