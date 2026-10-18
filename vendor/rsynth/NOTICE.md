# Notices and attributions

rsynth-nvda is other people's work with a thin layer of ours around it.
This file names every borrowed piece, who made it, and under what terms.
It ships inside the NVDA add-on as well.

## RSynth — Nick Ing-Simmons, 1994–2004

The synthesizer is RSynth, written by **Nick Ing-Simmons**. Every `.c`,
`.h` and `.def` file at the top of this repository is his, as he left it
in 2004, apart from the patches listed in
[docs/02-library-build.md](docs/02-library-build.md), each of which is
marked in the source with `rsynth-nvda P<n>`.

- The library files (`opsynth.c`, `holmes.c`, `elements.c`, `Elements.def`,
  `phtoelm.c`, `say.c`, `text.c`, `english.c`, `darray.c`, `trie.c`,
  `charset.c`, `phones.c`, … ) are under the **GNU Library General Public
  License, version 2 or later** — `COPYING.LIB`.
- The programs (`saymain.c`, `mkdictdb.c`, `dlookup.c`, …) are under the
  **GNU General Public License, version 2 or later** — `COPYING`.

Components Nick Ing-Simmons credited in his README:

- `text.c` / `english.c` — the US **Naval Research Laboratory**
  letter-to-sound rules (NRL Report 7948, Elovitz, Johnson, McHugh and
  Shore, 1976), from the version on the comp.speech archives, which the
  source describes as believed to be public domain. Number reading derives
  from the same source.
- `holmes.c` / `Elements.def` — his implementation of the phoneme-to-
  parameter scheme of Holmes, Mattingly and Shearme (1964), *Speech
  Synthesis by Rule*.
- `opsynth.c` — his re-implementation of the cascade/parallel formant
  synthesizer of Dennis H. Klatt (1980).
- `Elements.def` nasal vowel parameters from Franz-Albert Van den Bussche
  (ChangeLog.2004, change 3438).
- German rules (`deutsch.c`, not built here) by jms.
- The acknowledgements in `README` (Tony Robinson, Axel Belinfante, Jon
  Iles, Rob Hooft and the porters listed there) stand.

The oldest commits in this repository's history are Nick Ing-Simmons'
own import from Perforce (January 2004). The autotools commits of
2012–2013 in the same history are by **Reece H. Dunn**.

## CMU Pronouncing Dictionary — Carnegie Mellon University

`dict/cmudict/cmudict.dict` is the CMU Pronouncing Dictionary from
[cmusphinx/cmudict](https://github.com/cmusphinx/cmudict), commit
`74790861f652b15e4ac49015a90074ad62a27690` (2025-10-24), unchanged. The
build compiles it into the library (`tools/mkdicttab.c`), so the add-on's
`rsynth.dll` is a binary redistribution of it, and the add-on carries the
licence as `LICENSE-cmudict.txt`:

```
Copyright (C) 1993-2015 Carnegie Mellon University. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   The contents of this file are deemed to be source code.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

This work was supported in part by funding from the Defense Advanced
Research Projects Agency, the Office of Naval Research and the National
Science Foundation of the United States of America, and by member
companies of the Carnegie Mellon Sphinx Speech Consortium. We acknowledge
the contributions of many volunteers to the expansion and improvement of
this dictionary.

THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## NVDA

The driver in `nvda-addon/` is written against NVDA's synth driver API
(NV Access, GPL v2). It imports NVDA's modules at run time and copies none
of NVDA's code.

## This project's own contribution

`port/`, `include/`, `cli/`, `gui/`, `tests/`, `tools/mkdicttab.c` (which
is mkdictdb.c's parser with the database replaced, and stays GPL like it),
`nvda-addon/` and `docs/`. The library-side files and the GUI are offered
under the LGPL v2 or later like the engine they wrap; `cli/rsynth_say.c`
under the GPL v2 or later like the `say` it replaces. `rsynth_gui.exe`
links the engine and the compiled CMU dictionary in, so it carries both
sets of terms above; its About box names them.

The LGPL requires that users can get the library's source: it is this
repository, <https://github.com/dengopaiv/rsynth-nvda>.

The earlier pure-Python port, with its wxPython GUI, lives on the branch
`python-port` and is not part of this build.
