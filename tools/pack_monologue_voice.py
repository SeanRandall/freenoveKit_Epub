"""Convert a ProVoice speech-font DLL and dictionary into an ESP voice pack."""

import argparse
import pathlib
import struct
import sys


TYPE_BYTES = 24
NAME_BYTES = 40
ENTRY = struct.Struct(f"<BBHII{TYPE_BYTES}s{NAME_BYTES}sII")
HEADER = struct.Struct("<8sII")
MAGIC = b"MONOVC1\0"


def fixed(value, size):
    value = value.encode("ascii")
    if len(value) >= size:
        raise ValueError(f"resource name too long: {value!r}")
    return value + bytes(size - len(value))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("font", type=pathlib.Path)
    parser.add_argument("dictionary", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()

    sys.path.insert(0, str(pathlib.Path(__file__).parents[1] / "vendor" /
                           "provoice-reborn" / "tools"))
    from fbvconv import read_pe_resources

    resources = list(read_pe_resources(args.font).items())
    resources.append((("MONO_DICTIONARY", 1), args.dictionary.read_bytes()))
    table_size = HEADER.size + ENTRY.size * len(resources)
    offset = table_size
    entries = []
    payload = bytearray()
    for (kind, name), data in resources:
        type_numeric = isinstance(kind, int)
        name_numeric = isinstance(name, int)
        entries.append(ENTRY.pack(
            type_numeric, name_numeric, 0,
            kind if type_numeric else 0,
            name if name_numeric else 0,
            fixed("" if type_numeric else kind, TYPE_BYTES),
            fixed("" if name_numeric else name, NAME_BYTES),
            offset, len(data)))
        payload.extend(data)
        offset += len(data)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as output:
        output.write(HEADER.pack(MAGIC, len(entries), ENTRY.size))
        output.writelines(entries)
        output.write(payload)
    print(f"{args.output}: {len(resources)} resources, {offset} bytes")


if __name__ == "__main__":
    main()
