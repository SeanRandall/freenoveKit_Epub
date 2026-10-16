"""Generate src/frontend/fe_data.c and fe_data.h: the tables the C front end reads, extracted from TV_ENG32.DLL.

usage: python tools/fe_extract.py [TV_ENG32.DLL] [-o src/frontend]

Data policy (AGENTS.md): table contents live only in these generated files, never in hand-written sources or
REFERENCE.md. Each entry names the VA it came from. Add an entry to TABLES when a ported function needs one.
"""
import os
import struct
import sys

import pefile

BASE = 0x10000000

# (C name, kind, VA, comment)
#   'str'   one 0-terminated string at VA
#   'strs'  a list of VAs, one string each -> const char *const NAME[]
#   'pairs' (table VA, count VA) -> FePair NAME[count] of {string, string} pointer pairs
#   'shorts' (VA, n) -> const short NAME[n]
#   'strs2d' a list of lists of VAs -> const char *const NAME[n][max+1], each row NULL-terminated
#   'ptrs'  (table VA, count VA) -> const char *const NAME[count] from an array of string pointers
#   'triples' (table VA, n) -> FeTriple NAME[n] of {string, string, string} pointer triples
#   'bytes' (VA, n) -> const unsigned char NAME[n] (raw bytes: character-class tables, fixed-width records)
TABLES = [
    ('fe_mod_stopwords', 'strs',
     [0x100d1f08, 0x100d1ff0, 0x100d20d8, 0x100d2170, 0x100d2240, 0x100d2308, 0x100d2368, 0x100d23b0,
      0x100d2490, 0x100d24e8, 0x100d25c8, 0x100d26b0, 0x100d2798, 0x100d2880, 0x100d2968, 0x100d2a50,
      0x100d2b38, 0x100d2c20, 0x100d2d08, 0x100d2df0, 0x100d2ed8, 0x100d2fc0, 0x100d30a8, 0x100d3190],
     'Rule_MeasureModifiers 1004af00: "#word#...#" lists; a word in any of them ends the measure phrase'),
    ('fe_state_names', 'strs', [0x10100214, 0x10100134, 0x10100110],
     'US state names, "#alabama#...#", in three parts (Rule_Direction2 1006ebc0 and others)'),
    ('fe_person_names', 'strs2d',
     [[0x100d4120, 0x100d3908, 0x100d3230],
      [0x100d41c8, 0x100d42a8, 0x100d4390, 0x100d4478, 0x100d39b0, 0x100d32b8],
      [0x100d44a0, 0x100d4588, 0x100d4670, 0x100d3a10, 0x100d3318],
      [0x100d4720, 0x100d4800, 0x100d3a78, 0x100d33e8],
      [0x100d4878, 0x100d3af0, 0x100d3448],
      [0x100d48e0, 0x100d49c8, 0x100d3b48, 0x100d34b0],
      [0x100d4a40, 0x100d4b28, 0x100d3bb8, 0x100d34e0],
      [0x100d4bf8, 0x100d4ce0, 0x100d4dc8, 0x100d4eb0, 0x100d3c10, 0x100d3500],
      [0x100d4ec8, 0x100d3c60, 0x100d3528],
      [0x100d4ed8, 0x100d3c78, 0x100d3538],
      [0x100d4f60, 0x100d3d48, 0x100d35c8],
      [0x100d5030, 0x100d5118, 0x100d3d88, 0x100d3630],
      [0x100d5130, 0x100d5210, 0x100d52f0, 0x100d53d0, 0x100d3df0, 0x100d36b0],
      [0x100d5490, 0x100d3e80, 0x100d3770],
      [0x100d5508, 0x100d3ecc, 0x100d3794],
      [0x100d5590, 0x100d5678, 0x100d3ed0, 0x100d3798],
      [0x100d56f8, 0x100d3f40, 0x100d37c0],
      [0x100d5700, 0x100d57e8, 0x100d3f48, 0x100d37c8],
      [0x100d58c0, 0x100d59a8, 0x100d5a88, 0x100d5b68, 0x100d3fd8, 0x100d3808],
      [0x100d5bc8, 0x100d4048, 0x100d3888],
      [0x100d5c48, 0x100d408c, 0x100d38b8],
      [0x100d5c58, 0x100d4090, 0x100d38c0],
      [0x100d5cc0, 0x100d5da0, 0x100d40c0, 0x100d38e8],
      [0x100d5e7c, 0x100d40fc, 0x100d38f0],
      [0x100d5e80, 0x100d4100, 0x100d38f8],
      [0x100d5e98, 0x100d4110, 0x100d3900]],
     'IsPersonName 100586c0: per initial letter a-z, surname lists then male and female first names, NULL-terminated'),
    ('fe_city_state', 'pairs', (0x101069e8, 0x10108994),
     'city-state table: {state, city} pairs, lower case; count from the short at the second VA (823)'),
    ('fe_city_letter_end', 'shorts', (0x10108928, 53),
     'CityState_LetterRange 100402f0: for the k-th character of "A..Za..z{" (1-based), rows T[k-1]+1 .. T[k]'),
    ('fe_qual_table', 'pairs', (0x101059f0, 0x101069e0),
     'ClassifyWord 100330f0: {word, QUAL} pairs (n -> direction, ounce -> measure, ...); count from the short at the second VA'),
    ('fe_num_mods', 'ptrs', (0x10106990, 0x101069e4),
     'ClassifyWord 100330f0: number modifiers ("20+", "256K", "5#" = pounds)'),
    ('fe_abbrev', 'triples', (0x10108998, 902),
     'abbreviation table: {key (lower case, sorted), expansion, rule name}'),
    ('fe_abbrev_letter_end', 'shorts', (0x1010b878, 53),
     'Abbrev_LetterRange 10040280: for the k-th character of "A..Za..z{" (1-based), rows T[k-1]+1 .. T[k]; T[0] for others'),
    ('fe_acr_100a27b0', 'strs', [0x100a27b0, 0x100a282c, 0x100a2860, 0x100a28bc, 0x100a28dc, 0x100a2900],
     'Acronym_OnsetSplit 10019380: words that begin with a silent-letter onset: kn, gh, rh, ps, pt, gn ("#knack#...#")'),
    ('fe_acr_morphs', 'ptrs', (0x1010b8e8, 0x1010b9b0),
     'StripMorphs_A/B 1001d030/1001d410: morphs split off the front / back of an acronym ("dos", "net", ...)'),
    ('fe_acr_morph_exc', 'ptrs', (0x1010b9b8, 0x1010bc10),
     'StripMorphs_A/B 1001d030/1001d410: words whose prefix / suffix is not a morph ("dose", "inform", "magnet", ...)'),
    ('fe_acr_known', 'strs', [0x100a2140, 0x100a22d0, 0x100a2228, 0x100a2358, 0x100a2398, 0x100a2388, 0x100a23d8,
                              0x100a23e8, 0x100a2570],
     'Acronym_KnownLists 10016ae0: "#word#" lists: [0-2] spelled letter by letter, [3-4] first letter split off, '
     '[5-6] last letter split off, [7] kept as a word, [8] kept unless an upper-case neighbour'),
    ('fe_acr_respell_keys', 'str', 0x100a2410,
     'Acronym_KnownLists 10016ae0: respelled words, 8 characters each, space padded'),
    ('fe_acr_respell', 'str', 0x100a24b0,
     'Acronym_KnownLists 10016ae0: their respellings, 10 characters each, space padded'),
    ('fe_mil_ordinals', 'bytes', (0x100d1ee5, 30),
     'Acronym_MilitaryRating 100502f0: 3-character ordinals indexed by atoi of the grade digit (0: zero bytes)'),
    ('fe_acr_100a1de0', 'strs', [0x100a1de0, 0x100a1ea0, 0x100a1f30, 0x100a2000],
     'Acronym_SyllableParse 10017630: words where an initial c / g is soft ("#cern#...#", "#cid#...#") and where '
     'an initial ge / gi is hard ("#gear#...#", "#gila#...#")'),
    ('fe_nl_fractions', 'str', 0x100bd914,
     'NumLine_Fraction 10045320: denominators 1-19 ("one", "half", "third", ...), 11 characters each, space padded'),
    ('fe_nl_ordinals', 'str', 0x100bd8b0,
     'NumLine_Fraction 10045320: ordinals 1-9 ("first", ...), 11 characters each, space padded'),
    ('fe_nl_tens', 'str', 0x100bd854,
     'NumLine_Fraction 10045320: 10, 20 ... 90 as denominators ("one", "twentieth", ...), 10 characters each'),
    ('fe_nl_months', 'str', 0x100bd524,
     'NumLine_MonthAbbrev 100459e0, NumLine_Date 10045bd0: month names, 9 characters each, space padded'),
    ('fe_nl_months3', 'str', 0x100d1e74, 'NumLine_MonthAbbrev 100459e0: "JanFeb...", 3 characters each'),
]


def c_escape(b):
    out = []
    for c in b:
        ch = chr(c)
        if ch in '\\"':
            out.append('\\' + ch)
        elif 32 <= c < 127:
            out.append(ch)
        else:
            out.append('\\%03o' % c)
    return ''.join(out)


def cstring(img, va):
    o = va - BASE
    return img[o:img.index(b'\0', o)]


def main():
    args = sys.argv[1:]
    dll = 'TV_ENG32.DLL'
    outdir = os.path.join('src', 'frontend')
    if '-o' in args:
        outdir = args[args.index('-o') + 1]
        del args[args.index('-o'):args.index('-o') + 2]
    if args:
        dll = args[0]
    img = pefile.PE(dll).get_memory_mapped_image()
    c = ['/* Generated by tools/fe_extract.py from TV_ENG32.DLL. Do not edit. */', '#include "fe_data.h"', '']
    h = ['/* Generated by tools/fe_extract.py from TV_ENG32.DLL. Do not edit. */', '#ifndef FE_DATA_H',
         '#define FE_DATA_H', '', 'typedef struct { const char *a, *b; } FePair;',
         'typedef struct { const char *key, *exp, *rule; } FeTriple;', '']
    for name, kind, va, comment in TABLES:
        if kind == 'str':
            s = cstring(img, va)
            c += ['/* %s (@0x%08x) */' % (comment, va), 'const char %s[] = "%s";' % (name, c_escape(s)), '']
            h.append('extern const char %s[%d];' % (name, len(s) + 1))
        elif kind == 'strs':
            c.append('/* %s */' % comment)
            c.append('const char *const %s[%d] = {' % (name, len(va)))
            for a in va:
                c.append('    /* @0x%08x */ "%s",' % (a, c_escape(cstring(img, a))))
            c += ['};', '']
            h.append('extern const char *const %s[%d];' % (name, len(va)))
        elif kind == 'pairs':
            base, cnt_va = va
            n = struct.unpack_from('<h', img, cnt_va - BASE)[0]
            c.append('/* %s (@0x%08x, count @0x%08x) */' % (comment, base, cnt_va))
            c.append('const FePair %s[%d] = {' % (name, n))
            for k in range(n):
                a, b = struct.unpack_from('<II', img, base - BASE + 8 * k)
                c.append('    {"%s", "%s"},' % (c_escape(cstring(img, a)), c_escape(cstring(img, b))))
            c += ['};', 'const short %s_count = %d;' % (name, n), '']
            h.append('extern const FePair %s[%d];' % (name, n))
            h.append('extern const short %s_count;' % name)
        elif kind == 'ptrs':
            base, cnt_va = va
            n = struct.unpack_from('<h', img, cnt_va - BASE)[0]
            c.append('/* %s (@0x%08x, count @0x%08x) */' % (comment, base, cnt_va))
            c.append('const char *const %s[%d] = {' % (name, n))
            for k in range(n):
                c.append('    "%s",' % c_escape(cstring(img, struct.unpack_from('<I', img, base - BASE + 4 * k)[0])))
            c += ['};', 'const short %s_count = %d;' % (name, n), '']
            h.append('extern const char *const %s[%d];' % (name, n))
            h.append('extern const short %s_count;' % name)
        elif kind == 'triples':
            base, n = va
            c.append('/* %s (@0x%08x) */' % (comment, base))
            c.append('const FeTriple %s[%d] = {' % (name, n))
            for k in range(n):
                t = struct.unpack_from('<III', img, base - BASE + 12 * k)
                c.append('    {%s},' % ', '.join('"%s"' % c_escape(cstring(img, a)) for a in t))
            c += ['};', '']
            h.append('extern const FeTriple %s[%d];' % (name, n))
        elif kind == 'shorts':
            base, n = va
            vals = struct.unpack_from('<%dh' % n, img, base - BASE)
            c.append('/* %s (@0x%08x) */' % (comment, base))
            c.append('const short %s[%d] = {' % (name, n))
            for k in range(0, n, 16):
                c.append('    ' + ' '.join('%d,' % v for v in vals[k:k + 16]))
            c += ['};', '']
            h.append('extern const short %s[%d];' % (name, n))
        elif kind == 'bytes':
            base, n = va
            vals = img[base - BASE:base - BASE + n]
            c.append('/* %s (@0x%08x) */' % (comment, base))
            c.append('const unsigned char %s[%d] = {' % (name, n))
            for k in range(0, n, 16):
                c.append('    ' + ' '.join('0x%02x,' % v for v in vals[k:k + 16]))
            c += ['};', '']
            h.append('extern const unsigned char %s[%d];' % (name, n))
        elif kind == 'strs2d':
            w = max(len(r) for r in va) + 1
            c.append('/* %s */' % comment)
            c.append('const char *const %s[%d][%d] = {' % (name, len(va), w))
            for r in va:
                c.append('    {')
                for a in r:
                    c.append('        /* @0x%08x */ "%s",' % (a, c_escape(cstring(img, a))))
                c.append('        0},')
            c += ['};', '']
            h.append('extern const char *const %s[%d][%d];' % (name, len(va), w))
    h += ['', '#endif', '']
    open(os.path.join(outdir, 'fe_data.c'), 'w', newline='\n').write('\n'.join(c))
    open(os.path.join(outdir, 'fe_data.h'), 'w', newline='\n').write('\n'.join(h))
    print('wrote %d tables to %s' % (len(TABLES), outdir))


if __name__ == '__main__':
    main()
