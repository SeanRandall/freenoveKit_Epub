# Pronunciation substitutions

Retroreader replaces text immediately before it reaches the selected speech
engine. Files are UTF-8, tab-separated text: text to find in column one and its
replacement in column two. Do not use spaces in place of the tab.

```text
NVDA	N V D A
Dr.	doctor
```

Put files in `.rr`. `substitutions.tsv` applies to every voice. An optional
engine file loads afterwards and overrides a global entry with the same first
column. The combined limit is 160 entries.

| Engine | Engine-specific filename |
|---|---|
| OpenEVV | `substitutions.openevv.tsv` |
| DECtalk DTC-01 | `substitutions.dectalk.tsv` |
| DECtalk 4.3 | `substitutions.dectalk4.3.tsv` |
| DoubleTalk Hybrid | `substitutions.doubletalk.tsv` |
| Monologue | `substitutions.monologue.tsv` |
| Pico | `substitutions.pico.tsv` |
| OpenBST | `substitutions.openbst.tsv` |
| TruVoice | `substitutions.truvoice.tsv` |
| Apollo | `substitutions.apollo.tsv` |
| Prose 2000 | `substitutions.prose2000.tsv` |

Plain replacements belong in the global file. Phonemes and commands belong
only in the matching engine file. Restart Retroreader after editing one.

## Escapes for control-character engines

Replacement fields recognise `\\e` for Escape, `\\xNN` for a non-zero hex
byte, `\\r`, `\\n`, `\\t`, and `\\\\` for a literal backslash. They are
decoded only in column two. Thus `\\x01` produces DoubleTalk's Control-A and
`\\e[2I` produces Prose's phoneme-input command.

## OpenEVV: IBM Speech Pronunciation Rules

OpenEVV accepts IBM ViaVoice/Eloquence annotations and Speech Pronunciation
Rules (SPR). Enclose an SPR pronunciation in backquote-square brackets:

```text
though	`[.1Do]
shocking	`[.1Sa.0kIG]
construction	`[kXn1strHkSXn]
```

SPR is case-sensitive. `1` is primary stress, `2` secondary stress, `0`
unstressed, and `.` is a syllable boundary (optional in English).

| Set | Symbols |
|---|---|
| US vowels | `a A e E i I o c u U H R` |
| UK vowels | `a A e E i I o c @ u U H R` |
| Diphthongs | `O W Y` |
| Reduced vowels | `x X` |
| US consonants | `b p d t g k D T v f z s Z S J C h m n G r l w y M ? F N` |
| UK consonants | `b p d t g k D T v f z s Z S J C h m n G r l L y w` |
| Modifiers | `1 2 0 .` |

| Annotation | Effect |
|---|---|
| `` `v1 `` through `` `v8 `` | Select a preset voice |
| `` `vg<number> `` | Gender |
| `` `vh<number> `` | Head size |
| `` `vb<number> `` | Baseline pitch |
| `` `vf<number> `` | Pitch fluctuation |
| `` `vr<number> `` | Roughness |
| `` `vy<number> `` | Breathiness |
| `` `vs<number> `` | Speaking speed |
| `` `vv<number> `` | Voice volume |
| `` `0 `` through `` `4 ``, and `` `00 `` | Inline pause controls |

Annotations persist until changed, so restore values a replacement changes.
API parameter names such as `pp0` must not be placed in spoken text.

## DECtalk DTC-01

DTC-01's square-bracket phonemic mode is enabled by the engine. Put phonemes
directly in brackets, for example `[d'iyk]`. Its two-character alphabet is
case-insensitive.

| Set | Symbols |
|---|---|
| Vowels | `aa ae ah ao aw ax ay eh ey ih ix iy ow oy uh uw yu` |
| R-coloured vowels | `ir er ar or ur rr` |
| Syllabics | `el em en` |
| Consonants | `b ch d dh f g hx jh k l m n nx p r s sh t th v w y z zh` |
| Silence/allophones | `_ rx lx dx tx q` |

| Modifier | Meaning |
|---|---|
| `'`, backquote, `"` | Primary, secondary and emphatic stress; before vowel |
| `-`, `*`, `#`, space | Syllable, morpheme, compound and word boundaries |
| `, . ? !` | Clause endings |
| `phoneme<duration,pitch>` | Duration in ms and ending pitch in Hz |
| pitch `1` to `37` | Musical C2 through C5 with vibrato |

Stops `p t k b d g` cannot be sustained. Other bracket commands include `:ra
120` to `:ra 350`, `:cp` and `:pp` from 0 to 9000 ms, voices `:np :nb :nh
:nf :nk :nr :nu :nv`, and `:dv` voice-design parameters. DTC-01 does not use
the newer `[:phoneme arpabet ...]` command.

## DECtalk 4.3

When substitutions are loaded, Retroreader automatically prefixes every
DECtalk 4.3 utterance with `[:phoneme arpabet speak on]`. A replacement can
therefore contain bracketed ARPAbet directly.

| Set | Symbols |
|---|---|
| Vowels | `e a i E A I O o u ^ W Y R c @ U | x` |
| Consonants | `p t k f T s S C w y h l L N b d g v D z Z J m n G r q Q & F` |
| R-coloured vowels | `B K P M j` |
| Stress/boundaries | `'` primary, backquote secondary, `"` emphatic, `# ( ) * ~ -` |

Commands include `[:rate 200]`, `[:name paul]`, `[:volume 50]`, `[:comma
150]`, `[:period 300]`, `[:punct ...]`, `[:pronounce ...]`, `[:say ...]`,
`[:dv ...]`, and `[:tone ...]`. Avoid index, sync, play, logging and error
commands because Retroreader owns buffering and navigation.

## DoubleTalk Hybrid

Enter direct phoneme mode with `\\x01D`, separate every phoneme with a space,
and return to text mode with `\\x01T`.

```text
computer	\x01DK AX M P YY UW DX ER\x01T
```

| Set | Symbols |
|---|---|
| English vowels | `AA AE AH AW AX AY EH ER EY IH IX IY OW OY UH UW` |
| English consonants | `B CH D DH DX F G H J K KX L M N NG P PX R S SH T TH TX V W WH Y YY Z ZH` |
| Additional multilingual sounds | `A E EI EW I NY O RR U` |
| Pauses | apostrophe short, space variable, comma medium, period long |

Inside phoneme mode, `nn` sets pitch 0–99, `/` and backslash raise and lower
pitch temporarily, `+` and `-` alter speed, and `>` and `<` alter volume.
Ordinary commands also use `\\x01`, for example `\\x019S` for speed 9. Always
return to text mode in the same replacement.

## Pico

Pico accepts one-word X-SAMPA in its native compact tag:

```text
word	<phoneme ph='"w3:d'/>
```

The inventory is language-dependent. The useful English core is:

| Set | X-SAMPA symbols |
|---|---|
| Stops/fricatives/nasals | `p b t d k g f v T D s z S Z h m n N` |
| Other consonants | `t_S d_Z l r\\ w j` |
| US/UK vowels | `I E { V Q U @ 3 A i: u: O:` |
| Diphthongs | `e_I a_I O_I a_U @U I@ E@ U@` |
| Modifiers | `"` primary stress, `%` secondary, `.` syllable, `:` length, `=` syllabic |

Use the symbols of the installed Pico language. The compact tag, rather than a
full `<speak>` document, is what reaches Pico's low-level engine.

## TruVoice

In normal text mode, prefix each TruVoice phoneme symbol with `&`.

| Sound group | Symbols |
|---|---|
| Vowels/diphthongs | `A a b c E e f g I i O o U u v w y 3 4 5 @ |` |
| Consonants | `B C D d F G H J K L l M N n P q R S s T t V W X x Y Z z ~` |
| Stress | `1` primary, `2` secondary |

Examples: `&C` “ch”, `&s` “sh”, `&X` unvoiced “th”, `&x` voiced “th”, `&~`
“ng”, and `&@` schwa. Symbols are case-sensitive. Advanced users may use
`\\e[1I` for continuous phoneme mode and `\\e[0I` to return to text, but
ampersand phonemes are safer in a single replacement.

## Prose 2000

Prose 2000 1988 supports direct phoneme input. Use `\\e[2I`, the engine's
two-character phoneme names, then `\\e[0I` to return to text. Primary and
secondary stress are `1` and `2` after the vowel; `"` is emphatic stress. A
phoneme may be followed by `!duration;pitch` (or `/duration;pitch`) to override
timing and pitch. Keep the mode switch and reset in one replacement.

The 1983 Prose variant lacks the same documented phoneme-spelling front end;
use plain text substitutions there.

## Dolphin Apollo II

The Apollo port retains the original firmware's command parser. For a mixed
text-and-phoneme replacement, prefix every phoneme with an underscore. This is
safer than switching the entire input stream into phonetic mode.

```text
cat	_K_AE_T
again	_A_G_P_E_N
```

| Sound group | Phonemes |
|---|---|
| Vowels/diphthongs | `_AA _AI _AR _AW _A _EE _EI _ER _E _IA _IE _I _OA _00 _OR _OW _OY _O _UU _U` |
| Stops | `_B _D _G _K _KK _P _PP _T _TT` |
| Fricatives/affricates | `_CH _DH _F _H _J _SH _S _TH _V _X _ZH _Z` |
| Nasals/liquids/glides | `_L _M _N _NG _Q _QQ _R _W _Y` |

`_QQ` is a slight pause. `_KK`, `_PP` and `_TT` are the variants heard in
words such as “school”, “speech” and “steel”. The manual's `_00` uses two
zeroes rather than two letter O characters, as in “good”. Use `@X1` for an entirely phonetic run and
`@X0` to return to ordinary text; both switches must be in the same replacement.

Apollo also exposes its voice-shaping commands in replacements:

| Command | Effect |
|---|---|
| `@W1`–`@W9` | Speed, approximately 80–580 words per minute |
| `@F0`–`@FF`, `@F+`, `@F-` | Pitch centre or a small pitch step |
| `@R0`–`@R7` | Overall prosody/excitability |
| `@Q0`–`@Q9` | Inter-word pause |
| `@D0`–`@DF` | End-of-phrase pause |
| `@B1`–`@B8` | Voicing, from breathy whisper to fully voiced |
| `@M00`–`@M3F` | Mark–space ratio, affecting voice depth |
| `@K0`, `@K1` | Male or non-male formant table |
| `@$0`–`@$7` | Source/filter combination |
| `@V1`–`@V6` | Original preset voices; also resets custom formant offsets |
| `@A0`–`@AF` | Synthesis volume within an utterance |

The source/filter values are: 0 male/default, 1 non-male/default, 2 male/spike,
3 non-male/spike, 4 male/cut-down, 5 non-male/cut-down, 6 male/reduced-high,
and 7 non-male/reduced-high.

The advanced formant command is `@Udhh+` or `@Udhh-`, where `d` selects the
parameter and `hh` is a hexadecimal offset. Parameters are 0 nasal-formant
frequency, 1–3 formant frequencies F1–F3, 4 low-frequency amplitude, 5–8
amplitudes A1–A4, and 9 voicing. The fourth formant frequency is fixed.

These offsets persist in the Apollo firmware state. Undo an experimental
offset with the same value and opposite sign, or select a preset with `@V1`
through `@V6` to clear the offsets. Retroreader reapplies its ordinary rate,
pitch, expression and volume at the start of each utterance, but it does not
automatically erase formant offsets inserted by a substitution.

Apollo also accepts `@Rchh` to alter one of its internal prosody bytes (`c` is
`A` through `Z`, `hh` is hexadecimal). Those byte meanings are not named in
the user manual, so this low-level form is intentionally documented as
experimental rather than presented as 26 reliable user controls.

## Engines currently limited to plain replacements

The current Monologue and OpenBST adapters do not expose a validated
direct-phoneme syntax. Their engine-specific files still support ordinary
pronunciation spellings. This is an adapter limitation, not a claim that the
original products contain no lower-level phoneme data.
