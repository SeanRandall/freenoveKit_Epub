"""Copy TV_ENG32.DLL's resources into a .res file for the replacement DLL.

Usage: python tools/backend/gen_res.py <TV_ENG32.DLL> <out.res>

The replacement (src/dll) uses the original's dialogs, bitmaps, icon and
strings (the pronunciation manager of tts_UserDic, REFERENCE 10.6), and
keeps its version resource, so that it looks the same to a program that
reads it.  They are copied at build time and not committed (AGENTS.md,
data policy): every resource, with its type, name and language, byte for
byte.  tools/backend/build.sh turns the .res into an object with windres.
"""
import struct
import sys

import pefile


def sz_or_ord(v):
    """A .res type or name: an ordinal, or an upper-case UTF-16 string."""
    if isinstance(v, int):
        return struct.pack('<HH', 0xffff, v)
    return str(v).upper().encode('utf-16le') + b'\0\0'


def entry(rtype, name, lang, data, flags=0x1030):
    head = sz_or_ord(rtype) + sz_or_ord(name)
    head += b'\0' * (-len(head) % 4)
    head += struct.pack('<IHHII', 0, flags, lang, 0, 0)
    out = struct.pack('<II', len(data), 8 + len(head)) + head + data
    return out + b'\0' * (-len(out) % 4)


def main():
    dll, out = sys.argv[1], sys.argv[2]
    pe = pefile.PE(dll)
    res = [entry(0, 0, 0, b'', flags=0)]    # the empty entry a .res starts with
    n = 0
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        rtype = t.id if t.id is not None else str(t.name)
        for nm in t.directory.entries:
            name = nm.id if nm.id is not None else str(nm.name)
            for lg in nm.directory.entries:
                d = lg.data.struct
                res.append(entry(rtype, name, lg.id, pe.get_data(d.OffsetToData, d.Size)))
                n += 1
    with open(out, 'wb') as f:
        f.write(b''.join(res))
    print('resources: %d from %s' % (n, dll))


if __name__ == '__main__':
    main()
