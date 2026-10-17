/* Converted from the Apollo 2 ROM by tools/extract_tables.py (M4.0 spec section 4), then reviewed.
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM.
   Three bytes above $7F appear in the two-letter rules, as octal escapes. \220 ($90), among the characters a
   rule consumes (CAF\220, under CA), is E acute in code page 437 (cmd_case_map folds the lower-case $82 to it):
   lts_char_class has it a letter, so lts_match_symbol matches it against the text like any letter. The other
   two are in outputs, phonetic symbols that tn_append_phonetic copies into tn_phonetic like any other (their
   features and phoneme lists are in pros_data.c). \230 ($98), under ME, is a voiced stop (the G of MEGATON):
   its list, phonemes 21 and 23, is the only one with phoneme 23. \300 ($C0) is a long syllabic consonant (the
   final L of LEVEL, CANCEL): its list is phoneme 41 alone. */
#include <stddef.h>
#include "fw_data.h"


/* lts_rules_k at LOW $4010: KA */
static const fw_rule r_ka[] = {
    { "", "MPUCH", "EA", "KaMPUc" },
};
static const fw_rule_list l_ka = { r_ka, 1 };

/* lts_rules_k at LOW $4020: KE */
static const fw_rule r_ke[] = {
    { "#:", "D", " ", "KT" },
    { " ", "NY", "A", "KeNY" },
    { " ", "YB", " ", "KAE WIBE" },
};
static const fw_rule_list l_ke = { r_ke, 3 };

/* lts_rules_k at LOW $4041: KI */
static const fw_rule r_ki[] = {
    { "", "LO", "M", "CiLQ" },
    { "", "LO", ": ", "CELO" },
    { "", "LO", "", "CiLO" },
};
static const fw_rule_list l_ki = { r_ki, 3 };

/* lts_rules_k at LOW $405D: KN */
static const fw_rule r_kn[] = {
    { "", "OWLEDG", "", "NQLij" },
    { "", "OWS", "", "NOZ" },
    { "", "OW", ": ", "NO" },
    { " ", "OW", "#", "NO" },
    { "", "", "O!:", "N" },
};
static const fw_rule_list l_kn = { r_kn, 5 };

/* lts_rules_k at LOW $4089: KO */
static const fw_rule r_ko[] = {
    { " ", "R", "EA", "KxR" },
};
static const fw_rule_list l_ko = { r_ko, 1 };

static const fw_rule_list *const k_lts_rules_k[] = {
    &l_ka, &l_ke, &l_ki, &l_kn, &l_ko,
};
const fw_rule_table lts_rules_k = { "lts_rules_k", 0x4010, 131, true, k_lts_rules_k, 5 };

const fw_rule_list *const lts_pairs_k[26] = {
    ['A' - 'A'] = &l_ka,
    ['E' - 'A'] = &l_ke,
    ['I' - 'A'] = &l_ki,
    ['N' - 'A'] = &l_kn,
    ['O' - 'A'] = &l_ko,
};
/* lts_rules_l at LOW $4093: LA */
static const fw_rule r_la[] = {
    { "", "BE", "L", "LABx" },
    { "", "BORATOR", "", "LxBQRxTxR" },
    { "", "BOUR", "", "LABx|" },
    { "", "NCA", "SH", "Lankx" },
    { "", "NCA", "ST", "LanKa" },
    { ":#:", "N", "D", "LxN" },
    { "", "NGUOR", "", "LanGx|" },
    { " ", "OS", "", "LwS" },
    { " ", "S", "ER", "LAZ" },
    { " ", "SS", "", "LaS" },
    { " ", "TIN", "", "LaTiN" },
    { " ", "UGH", "", "LhF" },
    { " ", "WY", "ER", "LoY" },
    { " ", "Z", "+", "LAZ" },
};
static const fw_rule_list l_la = { r_la, 14 };

/* lts_rules_l at LOW $4128: LE */
static const fw_rule r_le[] = {
    { "", "A", "D", "LE" },
    { "", "AGU", "E", "LEG" },
    { "", "ASE", "", "LES" },
    { " ", "ATH", "+", "Led" },
    { " ", "BAN", "", "LeBxN" },
    { "", "G", "AL", "LEG" },
    { "", "GEND", "", "LejxND" },
    { "", "", "GION", "LE" },
    { " ", "I", "CES", "Le" },
    { "", "I", "SURE", "Le" },
    { " ", "ON", "+", "LbON" },
    { " ", "P", "ER", "LeP" },
    { ":#:", "SS", " ", "LxS" },
    { "", "SS", "", "LeS" },
    { " ", "SOTHO", "", "LxSUTU" },
    { " ", "", "ST ", "Le" },
    { "", "TTUCE", "", "LeTiS" },
    { "", "TH", "AL", "LEt" },
    { "", "VEL", "", "LeV\300" },
    { " ", "V", "ER", "LEV" },
    { "IS", "", " #", "L" },
    { "#:!", "", "CT", "Le" },
    { "#!!", "", "!:", "\300" },
    { "#:B", "", "!:", "\300" },
};
static const fw_rule_list l_le = { r_le, 24 };

/* lts_rules_l at LOW $4205: LI */
static const fw_rule r_li[] = {
    { "", "BE", "RIA", "LIBJ" },
    { "", "B", "ER", "LiB" },
    { " ", "BR", "ARIA", "LIBR" },
    { " ", "BRAR", "", "LIBRxR" },
    { "", "CENS", "", "LISxNS" },
    { "", "EUTEN", "ANT", "LxFTeN" },
    { " :", "E", "", "LI" },
    { "", "FE", "", "LIF" },
    { " ", "MA", " ", "LEMx" },
    { "", "ME", "", "LIM" },
    { " ", "MI", "T", "LiMi" },
    { "", "NEAR", "", "LiNJ|" },
    { "", "NE", "N: ", "LiNi" },
    { ":!#:", "N", "% ", "LIN" },
    { ":!A", "", " ", "LI" },
    { " ", "ON", "", "LIxN" },
    { "", "ON", "", "LYxN" },
    { " ", "QUI", "", "Li6Wi" },
    { " ", "QU", "OR", "LiK" },
    { " ", "", "QU", "Li" },
    { "", "ST", "EN", "LiS" },
    { " ", "TERA", "!", "LiTRx" },
    { " ", "THU", "AN", "LitYU" },
    { " ", "V", "ERPO", "LiV" },
    { "#:", "VEN", "", "LIV7" },
    { " ", "V", "%", "LiV" },
};
static const fw_rule_list l_li = { r_li, 26 };

/* The LOOP rule no entry reaches: an observed ROM bug (parent spec rev 6, section 1.2). The port keeps it unreachable. */
/* lts_rules_l at LOW $4310: no pair entry points here */
static const fw_rule r_l_unref_4310[] = {
    { "", "O", "P", "Lx" },
};
static const fw_rule_list l_l_unref_4310 = { r_l_unref_4310, 1 };

/* L-M and L-O share this list; see the LOOP list above. */
/* lts_rules_l at LOW $4317: LM, LO */
static const fw_rule r_lm_lo[] = {
    { "", "", "C#", "LO" },
    { " ", "GO", ": ", "LOGO" },
    { "", "G", "OUS", "LxG" },
    { "", "GUE", "", "LQG" },
    { "", "G", "Y", "Lxj" },
    { "", "G", "+", "LQj" },
    { "", "LL", "", "LQL" },
    { "", "NG", "E", "LQnG" },
    { " ", "OS", "E", "LUS" },
    { "#:", "PED", "", "LxPf" },
    { " ", "S", "%", "LUZ" },
    { " ", "", "ST", "LQ" },
    { "", "UNG", "+", "LwNj" },
    { " ", "V", "ER", "LuV" },
    { "", "W", "", "LO" },
};
static const fw_rule_list l_lm_lo = { r_lm_lo, 15 };

/* lts_rules_l at LOW $4393: LT */
static const fw_rule r_lt[] = {
    { " ", "D", " ", "LiMiTiD" },
};
static const fw_rule_list l_lt = { r_lt, 1 };

/* lts_rules_l at LOW $43A0: LU */
static const fw_rule r_lu[] = {
    { "", "", "BR", "LU" },
    { "", "", "D+", "LU" },
    { "", "", "S#", "LU" },
    { ":#", "", "T+", "LU" },
    { " ", "X", "U", "Luks" },
};
static const fw_rule_list l_lu = { r_lu, 5 };

/* lts_rules_l at LOW $43C4: LY */
static const fw_rule r_ly[] = {
    { "", "", "ING", "LI" },
    { " ", "R", "", "LiR" },
    { "", "SIS", "", "LxSiS" },
    { "", "SES", "", "LxSEZ" },
    { "", "", "ST", "Li" },
    { "", "S", "", "LIZ" },
    { "", "", "T% ", "LI" },
    { "", "", " ", "Lb" },
};
static const fw_rule_list l_ly = { r_ly, 8 };

static const fw_rule_list *const k_lts_rules_l[] = {
    &l_la, &l_le, &l_li, &l_l_unref_4310, &l_lm_lo, &l_lt,
    &l_lu, &l_ly,
};
const fw_rule_table lts_rules_l = { "lts_rules_l", 0x4093, 876, true, k_lts_rules_l, 8 };

const fw_rule_list *const lts_pairs_l[26] = {
    ['A' - 'A'] = &l_la,
    ['E' - 'A'] = &l_le,
    ['I' - 'A'] = &l_li,
    ['M' - 'A'] = &l_lm_lo,
    ['O' - 'A'] = &l_lm_lo,
    ['T' - 'A'] = &l_lt,
    ['U' - 'A'] = &l_lu,
    ['Y' - 'A'] = &l_ly,
};

/* lts_rules_m at LOW $43FF: MA */
static const fw_rule r_ma[] = {
    { "", "CHIN", "", "MxsEN" },
    { "", "CEDON", "IA", "MaSiD}ON" },
    { " ", "DAGA", "SC", "MaDxG}a" },
    { "", "DA", "M", "MaDx" },
    { "", "GAZ", "INE", "MaGxZ" },
    { "", "GENT", "A", "MxjeNT" },
    { "", "JORI", "T", "MxjQRx" },
    { "", "J", "OR", "MAj" },
    { " ", "LE", "", "MA1" },
    { " ", "LI", " ", "MhLb" },
    { " ", "L", "TR", "Ma1" },
    { "", "LT", "", "Mo1T" },
    { " ", "", "L", "Ma" },
    { "", "MA", "", "MaMx" },
    { " ", "", "NCHEST", "Ma" },
    { "#:", "ND", "", "M}hND" },
    { "", "NG", "ER", "MANj" },
    { "", "NIFEST", "", "MaNiFeSf" },
    { " ", "NIP", "UL", "MxNiP" },
    { "", "N", "TLE", "MaN" },
    { " ", "NU", "", "MaNYv" },
    { "", "P", "LE", "MAP" },
    { "", "RG", "AR", "Mhj" },
    { "", "RG", "+", "Mhj" },
    { "", "RTYR", "", "MhTx|" },
    { "", "RIN", "ER", "MaRiN" },
    { "", "R", "INE", "MxR" },
    { "", "SSAGE", "", "MaShz" },
    { " ", "SS", "", "MaS" },
    { "", "", "TERI", "Mx" },
    { "", "THE", "M", "Matx" },
    { "", "TINEE", "", "MaTiNA" },
    { "", "TR", "IX", "MAT3" },
    { "", "TRICES", "", "MAT3iSEZ" },
    { "", "TR", "ON", "MAT3" },
    { "", "TUR", "+", "MxcU|" },
    { "", "YBE", " ", "MABE" },
};
static const fw_rule_list l_ma = { r_ma, 37 };

/* lts_rules_m at LOW $458A: MB */
static const fw_rule r_mb[] = {
    { "", "", "ING: ", "M" },
    { "", "", "ER: ", "MB" },
    { "", "S", " ", "MZ" },
    { "", "", " ", "M" },
};
static const fw_rule_list l_mb = { r_mb, 4 };

/* lts_rules_m at LOW $45A5: MC */
static const fw_rule r_mc[] = {
    { " ", "A", " ", "eMSEA" },
    { " ", "", "", "MxK" },
};
static const fw_rule_list l_mc = { r_mc, 2 };

/* lts_rules_m at LOW $45B6: ME */
static const fw_rule r_me[] = {
    { " ", "", " ", "~ME" },
    { " ", "AS", "L", "MEZ" },
    { "", "A", "D", "ME" },
    { "", "", "AND", "ME" },
    { "", "A", "S", "Me" },
    { " ", "A", "NT ", "Me" },
    { "", "AN", "", "MEN" },
    { "", "D", "I#", "MED" },
    { "", "GATON", "", "Me\230xTuN" },
    { " ", "GA", "", "Me\230x" },
    { "", "LANCH", "", "MeLxnK" },
    { " ", "NU", "", "MeNYU" },
    { " ", "RE", "", "MJ|" },
    { "", "SON", "", "MEZQN" },
    { " ", "SO", "", "MESO" },
    { "", "XIC", "", "MekSiK" },
    { "", "T", "AL", "MeT" },
    { ":#:#:", "T", "ER", "MbT" },
    { "", "T", "ER", "MET" },
    { "", "T", "IC", "MxT" },
    { "", "TR", "+", "METx|" },
};
static const fw_rule_list l_me = { r_me, 21 };

/* lts_rules_m at LOW $4675: MI */
static const fw_rule r_mi[] = {
    { " ", "CRO", "M", "MIKRx" },
    { " ", "CRO", "SOFT", "M}IKRO" },
    { " ", "CR", "O", "MIKR" },
    { "", "CHAE", "L", "MIKx" },
    { "", "GR", "A", "MIGR" },
    { "", "", "LE", "MI" },
    { "", "LI", "T", "MiLi" },
    { "", "LLI", "", "MiLi" },
    { "", "N", "ERAL", "MiN" },
    { "", "NE", "", "MIN" },
    { "", "N", "%", "MIN" },
    { " ", "N", "I", "MiN" },
    { "", "RA", "C", "MiRa" },
    { "", "RAG", "+", "MiRhz" },
    { "", "", "RROR", "Mi" },
    { "", "SER", "#", "MiZxR" },
    { "", "SOGYN", "", "{MISQjiN" },
    { "", "SS", "ION", "Mis" },
    { " ", "S", "", "MiS" },
    { "", "", "TALK", "MI" },
    { " ", "NU", "T", "MiNi" },
};
static const fw_rule_list l_mi = { r_mi, 21 };

/* lts_rules_m at LOW $4741: MK */
static const fw_rule r_mk[] = {
    { " ", "DIR", " ", "eMKA DEIh|" },
};
static const fw_rule_list l_mk = { r_mk, 1 };

/* lts_rules_m at LOW $4753: ML */
static const fw_rule r_ml[] = {
    { " ", "LE", " ", "MaDxMWxZe1" },
};
static const fw_rule_list l_ml = { r_ml, 1 };

/* lts_rules_m at LOW $4764: MN */
static const fw_rule r_mn[] = {
    { " ", "E", "", "Ni" },
};
static const fw_rule_list l_mn = { r_mn, 1 };

/* lts_rules_m at LOW $476B: MO */
static const fw_rule r_mo[] = {
    { "", "B", "ILE: ", "MOB" },
    { "", "DE", "L", "MQDx" },
    { "", "D", "ERA", "MQD" },
    { "", "DERN", "", "MQD7" },
    { "", "DE", "ST", "MQDi" },
    { " ", "", "DIF", "MQ" },
    { "", "IST", "EN", "MyS" },
    { "", "NAR", "CH", "MQNx" },
    { " ", "N", "EY", "MuN" },
    { "", "NG", "ER", "MunG" },
    { " ", "N", "IE", "MuN" },
    { "", "NIT", "OR", "MQNiT" },
    { "", "NSIEUR", "", "MxSYr|" },
    { " ", "N", "O", "MQN" },
    { "", "N", "UM", "MQN" },
    { "", "O", "D", "MU" },
    { "", "R", "AL", "MQR" },
    { " ", "RE", "", "Mo|" },
    { "", "RI", "B", "MQRi" },
    { "", "RO", "CC", "MxRQ" },
    { "", "SC", "OW", "MQSK" },
    { "", "SQUIT", "", "MxSkET" },
    { " ", "", "ST", "MO" },
    { "", "TIVE", "", "MOTiV" },
    { " ", "UL", "D", "MO1" },
    { "", "UNTAIN", "", "MwNTxN" },
    { "", "USE", "", "MwS" },
    { "", "USTACHE", "", "{MuSfas" },
    { "RTS", "UTH", "", "Mxt" },
    { "AR", "UTH", "", "Mxt" },
    { "EY", "UTH", "", "Mxt" },
    { "", "V", "", "MUV" },
    { "", "W", "", "MO" },
    { "", "", "ZAMB", "MO" },
};
static const fw_rule_list l_mo = { r_mo, 34 };

/* lts_rules_m at LOW $48B3: MU */
static const fw_rule r_mu[] = {
    { "", "LTI", "", "Mu1Ti" },
    { "", "RMUR", "", "MrMx|" },
    { "", "S", "EUM", "MYvZ" },
    { " ", "SC", "LE", "MuS" },
};
static const fw_rule_list l_mu = { r_mu, 4 };

/* lts_rules_m at LOW $48DD: MY */
static const fw_rule r_my[] = {
    { " ", "", " ", "~MI" },
    { " ", "", "RIA", "Mi" },
    { "", "STER", "IOUS", "{MiSfJR" },
};
static const fw_rule_list l_my = { r_my, 3 };

static const fw_rule_list *const k_lts_rules_m[] = {
    &l_ma, &l_mb, &l_mc, &l_me, &l_mi, &l_mk,
    &l_ml, &l_mn, &l_mo, &l_mu, &l_my,
};
const fw_rule_table lts_rules_m = { "lts_rules_m", 0x43FF, 1279, true, k_lts_rules_m, 11 };

const fw_rule_list *const lts_pairs_m[26] = {
    ['A' - 'A'] = &l_ma,
    ['B' - 'A'] = &l_mb,
    ['C' - 'A'] = &l_mc,
    ['E' - 'A'] = &l_me,
    ['I' - 'A'] = &l_mi,
    ['K' - 'A'] = &l_mk,
    ['L' - 'A'] = &l_ml,
    ['N' - 'A'] = &l_mn,
    ['O' - 'A'] = &l_mo,
    ['U' - 'A'] = &l_mu,
    ['Y' - 'A'] = &l_my,
};

/* lts_rules_n at LOW $48FE: NA */
static const fw_rule r_na[] = {
    { " ", "ME", "", "NAM" },
    { " ", "MIB", "IA", "NxMiB" },
    { "", "KE", "D", "NAKi" },
    { " ", "S", "AL", "NAZ" },
    { " ", "T", "IVE", "NAT" },
    { "", "TURA", "L", "NacxRx" },
    { "", "T", "URE", "NAc" },
    { " ", "Z", "I", "NhTZ" },
};
static const fw_rule_list l_na = { r_na, 8 };

/* lts_rules_n at LOW $494F: NE */
static const fw_rule r_ne[] = {
    { "", "ATH", "", "NEt" },
    { "", "AR", "", "NJ|" },
    { "", "CESSI", "", "NxSeSi" },
    { "", "CESS", "", "NeSxS" },
    { "", "GLIGEE", "", "NeGLizA" },
    { "", "GRO", "", "NEGRO" },
    { " ", "", "O", "NE" },
    { " ", "PAL", "ESE", "NePxL" },
    { " ", "PAL", "", "NiPoL" },
    { " ", "TH", "ER", "Ned" },
    { " ", "VER", "", "~NeVx|" },
    { "", "UR", "", "NYUR" },
};
static const fw_rule_list l_ne = { r_ne, 12 };

/* lts_rules_n at LOW $49CD: NG */
static const fw_rule r_ng[] = {
    { "", "", "L%", "nG" },
    { "", "", "R", "nG" },
    { "#", "", " ", "n" },
    { "", "S", " ", "nZ" },
    { "", "U", "#", "nGW" },
    { "E", "", "+", "Nj" },
    { "", "", "EON", "Nj" },
    { "#", "", "%", "n" },
    { "", "", "#", "nG" },
    { "", "", "", "n" },
};
static const fw_rule_list l_ng = { r_ng, 10 };

/* lts_rules_n at LOW $4A05: NI */
static const fw_rule r_ni[] = {
    { " ", "CARAGU", "A", "NiKxRaGYU" },
    { " ", "C", "E", "NIS" },
    { "", "CHE", "", "NEs" },
    { " ", "GER", " ", "NEjl" },
    { "", "NE", "", "NIN" },
    { "", "NTH", "", "NINt" },
    { "", "ON", "", "NYxN" },
    { "", "TR", "#", "NIT3" },
};
static const fw_rule_list l_ni = { r_ni, 8 };

/* lts_rules_n at LOW $4A55: NK */
static const fw_rule r_nk[] = {
    { "", "", " ", "nk" },
    { "", "", "", "nK" },
};
static const fw_rule_list l_nk = { r_nk, 2 };

/* lts_rules_n at LOW $4A5F: NL */
static const fw_rule r_nl[] = {
    { " ", "SFUNC", " ", "eNeLeS FunK" },
};
static const fw_rule_list l_nl = { r_nl, 1 };

/* lts_rules_n at LOW $4A74: NO */
static const fw_rule r_no[] = {
    { "", "", "BLE", "NO" },
    { "", "", "BODY", "NO" },
    { " ", "IS", "", "NyZ" },
    { "", "", "L", "NQ" },
    { "O", "M", "+", "NxM" },
    { "", "M", "IN", "NQM" },
    { " ", "N", "ETH", "~NuN" },
    { " ", "N", "", "NQN" },
    { " ", "R", " ", "~No|" },
    { " ", "RW", "ICH", "NQR" },
    { "", "T", "IC+", "NOT" },
    { "", "T", "%", "NOT" },
    { "", "TH", "ER", "Nud" },
    { "", "TH", "", "Nut" },
    { "", "UGAT", "", "NUGh" },
    { "", "UR", "ISH", "NuR" },
    { " ", "VEMB", "ER", "NxVeMB" },
    { " ", "VEL", "", "NQV\300" },
    { " ", "W", " ", "Nw" },
};
static const fw_rule_list l_no = { r_no, 19 };

/* lts_rules_n at LOW $4B1B: NU */
static const fw_rule r_nu[] = {
    { "", "CL", "E#", "NYUKL" },
    { "", "", "NEATON", "Nu" },
    { "", "", "TRI", "NYU" },
    { " ", "C", "L", "NYU6" },
};
static const fw_rule_list l_nu = { r_nu, 4 };

static const fw_rule_list *const k_lts_rules_n[] = {
    &l_na, &l_ne, &l_ng, &l_ni, &l_nk, &l_nl,
    &l_no, &l_nu,
};
const fw_rule_table lts_rules_n = { "lts_rules_n", 0x48FE, 580, true, k_lts_rules_n, 8 };

const fw_rule_list *const lts_pairs_n[26] = {
    ['A' - 'A'] = &l_na,
    ['E' - 'A'] = &l_ne,
    ['G' - 'A'] = &l_ng,
    ['I' - 'A'] = &l_ni,
    ['K' - 'A'] = &l_nk,
    ['L' - 'A'] = &l_nl,
    ['O' - 'A'] = &l_no,
    ['U' - 'A'] = &l_nu,
};

/* lts_rules_o at LOW $4B42: OA */
static const fw_rule r_oa[] = {
    { "", "CH", "", "Oc" },
    { "", "R", "", "o|" },
    { " ", "", "S+S", "OA" },
    { "", "T", "", "OT" },
    { "", "", "", "O" },
};
static const fw_rule_list l_oa = { r_oa, 5 };

/* lts_rules_o at LOW $4B5E: OB */
static const fw_rule r_ob[] = {
    { " ", "", "AN ", "OB" },
    { "", "LIGE", "", "xBLIj" },
    { "", "", "AT: ", "xB" },
    { "", "", "ATIC", "xB" },
    { "", "ED", "+", "xBED" },
    { "", "", "TAIN", "xB" },
    { "", "", "TRU", "xB" },
};
static const fw_rule_list l_ob = { r_ob, 7 };

/* lts_rules_o at LOW $4B9A: OC */
static const fw_rule r_oc[] = {
    { "", "H", "URE", "Os" },
    { "", "C", "AS", "xK" },
    { "", "CUPIE", "", "QCYvPI" },
    { "", "CUPY", "", "QCYvPI" },
    { "", "CUR", ":", "{QKr|" },
    { "", "HEM", "", "OKeM" },
    { "", "", "ENTRIC", "xS" },
    { "", "LI", "TE", "xkLI" },
    { "", "R", "ATIC", "xKR" },
    { " ", "R", " ", "O SE h|" },
    { "", "TOB", "ER", "QKT}OB" },
};
static const fw_rule_list l_oc = { r_oc, 11 };

/* lts_rules_o at LOW $4C0E: OD */
static const fw_rule r_od[] = {
    { "", "", "ESIC", "OD" },
    { "", "ES", "Y", "QDxS" },
    { ":#:", "", "OLOGY", "xD" },
    { ":#:", "", ": ", "xD" },
    { "", "", "OX", "xD" },
    { "", "ROME", "", "xD3OM" },
    { "", "YNE", "", "ODIN" },
};
static const fw_rule_list l_od = { r_od, 7 };

/* lts_rules_o at LOW $4C4F: OE */
static const fw_rule r_oe[] = {
    { "", "OPATHIS", "", "EYxPatiS" },
    { "", "OPATH", "", "YxPat" },
};
static const fw_rule_list l_oe = { r_oe, 2 };

/* lts_rules_o at LOW $4C6D: OF */
static const fw_rule r_of[] = {
    { " ", "", " ", "~QV" },
    { "", "", " ", "xV" },
    { "", "", "!", "QF" },
};
static const fw_rule_list l_of = { r_of, 3 };

/* lts_rules_o at LOW $4C7F: OG */
static const fw_rule r_og[] = {
    { "", "AM", "", "QGxM" },
    { "", "ENISE", "", "QjxNIZ" },
    { "", "EN", "", "OjxN" },
    { "", "OM", "", "QGxM" },
    { "", "ON", "AL", "QGxN" },
    { "", "R", "APHIC", "xGR" },
    { "", "RAPH", "Y", "QGRxF" },
    { "", "RAPH", "IES", "QGRxF" },
    { "", "RAPH", "%", "QGRxF" },
    { "", "RAPH", "", "xGRhF" },
    { "", "RAM", "", "xGRaM" },
};
static const fw_rule_list l_og = { r_og, 11 };

/* lts_rules_o at LOW $4CF5: OH */
static const fw_rule r_oh[] = {
    { "", "", " ", "O" },
};
static const fw_rule_list l_oh = { r_oh, 1 };

/* lts_rules_o at LOW $4CFA: OI */
static const fw_rule r_oi[] = {
    { "", "NG", "", "Oin" },
    { "", "R", "", "Wh|" },
    { "E", "S", " ", "Wh" },
    { "E", "SE", " ", "WhZ" },
    { "", "", "", "y" },
};
static const fw_rule_list l_oi = { r_oi, 5 };

/* lts_rules_o at LOW $4D1B: OK */
static const fw_rule r_ok[] = {
    { " ", "", " ", "OCA" },
};
static const fw_rule_list l_ok = { r_ok, 1 };

/* lts_rules_o at LOW $4D23: OL */
static const fw_rule r_ol[] = {
    { "", "ITH", "", "OLit" },
    { "", "", "OGIC", "xL" },
    { "", "OG", "OUS", "QLxj" },
    { "#:", "OGUE", "", "OLQG" },
    { "", "OG", "IAN", "xLOj" },
    { "", "OG", "", "QLxj" },
    { ":", "", "D", "O1" },
    { "", "", "K", "O" },
    { "", "T", "", "O1T" },
    { "", "", "L% ", "O" },
    { "", "", "L ", "O" },
};
static const fw_rule_list l_ol = { r_ol, 11 };

/* lts_rules_o at LOW $4D7A: OM */
static const fw_rule r_om[] = {
    { "#:", "", "AL", "QM" },
    { " ", "AN", " ", "{OMhN" },
    { " ", "ANI", " ", "{OMhNb" },
    { "", "ANC", "Y", "xMaNS" },
    { "", "", "ETRIC", "xM" },
    { "", "ET", "R", "QMeT" },
    { "", "ET", "ER", "QMET" },
    { "", "IN", "OUS", "}QMiN" },
    { "", "ORPH", "", "OMoF" },
    { "", "PLY", "", "xMPLI" },
    { "", "PLIAN", "", "xMPLIxN" },
    { "#!!", "", " ", "m" },
    { "#:!", "", "", "uM" },
};
static const fw_rule_list l_om = { r_om, 13 };

/* lts_rules_o at LOW $4DFE: ON */
static const fw_rule r_on[] = {
    { " ", "", " ", "~QN" },
    { " ", "CE", "", "WuNS" },
    { ":#:!", "", "D", "xN" },
    { " ", "E", "", "WuN" },
    { ":#:!", "E", "D", "xN" },
    { "", "ERA", "T", "QNxRA" },
    { "#", "E", "", "WQN" },
    { "G", "E", "", "QN" },
    { "D", "E", "", "uN" },
    { "", "", "E", "ON" },
    { " ", "GO", "", "QNGO" },
    { "", "IA", "L", "ONJ" },
    { "", "IA", "N", "ONJ" },
    { "", "", "ION", "uN" },
    { "", "", "IS", "QN" },
    { " ", "", "LY", "ON" },
    { " ", "TO", " ", "~QNTU" },
    { "", "'", "T", "ON" },
    { "", "YM", "OUS", "QNiM" },
    { "", "YM", ": ", "xNiM" },
    { "", "YM", "Y", "QNxM" },
    { " !", "", "Y ", "ON" },
    { "C", "", "# ", "ON" },
    { "I", "", "", "xN" },
    { "+L", "", " ", "QN" },
    { "#:", "", "S ", "xN" },
    { "#:", "", "'S ", "xN" },
    { "#:", "", " ", "xN" },
};
static const fw_rule_list l_on = { r_on, 28 };

/* lts_rules_o at LOW $4EDB: OO */
static const fw_rule r_oo[] = {
    { "", "R", "", "q|" },
    { "S", "", "T", "v" },
    { "F", "", "T", "v" },
    { "", "", "DLE", "U" },
    { "G", "", "D", "v" },
    { "H", "", "D", "v" },
    { "", "", "K", "v" },
    { "", "N", "", "}UN" },
    { "", "", "", "U" },
};
static const fw_rule_list l_oo = { r_oo, 9 };

/* lts_rules_o at LOW $4F08: OP */
static const fw_rule r_op[] = {
    { "", "AE", "D", "xPE" },
    { "", "", "AQUE", "OP" },
    { "", "", "ATHIC", "OP" },
    { "", "", "ERA", "QP" },
    { "", "HOB", "+", "xFOB" },
    { "", "H", "YSIC", "OF" },
    { "", "HY", "TE", "OFI" },
    { "", "HONE", "", "xFON" },
    { "", "HON", "OUS", "QFxN" },
    { "", "HON", "Y", "QFxN" },
    { "", "HTHALM", "", "QFta1M" },
    { "", "", "OL", "QP" },
    { " ", "PORT", "#", "QPxT" },
    { "", "S", "+", "QpS" },
};
static const fw_rule_list l_op = { r_op, 14 };

/* lts_rules_o at LOW $4F8C: OR */
static const fw_rule r_or[] = {
    { " ", "", " ", "~o|" },
    { " ", "CHE", "STR", "oCe" },
    { "", "G", "AN", "oG" },
    { "", "OUGH", "", "uRx" },
    { "", "A", "TION", "oRA" },
    { "", "AR", "Y", "RxR" },
    { "", "AR", "ILY", "RxR" },
    { "", "AR", "IES", "xRaR" },
    { "#:", "", "IES ", "xR" },
    { "#:", "", "Y ", "xR" },
    { " ", "", "IG", "QR" },
    { "", "LE", "ANS", "oLb" },
    { " ", "ION", ": ", "QRIxN" },
    { " :", "", "+", "o|" },
    { "#:", "", "IC", "QR" },
    { "", "I", "T+", "QRi" },
    { "#:", "", "+", "o|" },
    { "#:", "", " ", "x|" },
    { "#:", "S", " ", "xZ" },
    { " ", "ANGE", "", "QRiNj" },
    { "", "", "", "o|" },
};
static const fw_rule_list l_or = { r_or, 21 };

/* lts_rules_o at LOW $5044: OS */
static const fw_rule r_os[] = {
    { "", "CELES", "", "QSxLEZ" },
    { "", "CO", "PE", "xSkO" },
    { "", "COP", "Y", "QSKxP" },
    { "", "ES", " ", "OSEZ" },
    { "", "IS", " ", "OSiS" },
    { "", "OPH", "ER", "QSxF" },
    { "", "", "OPHIC", "xS" },
    { "", "OPH", "", "QSxF" },
    { "", "", "PHERIC", "xS" },
    { "", "PHERE", "", "xSFJ|" },
    { "", "S", " ", "QS" },
    { "", "TE", "O", "QSfE" },
    { "", "", "URE: ", "Oz" },
    { "#:", "", " ", "OZ" },
};
static const fw_rule_list l_os = { r_os, 14 };

/* lts_rules_o at LOW $50CB: OT */
static const fw_rule r_ot[] = {
    { "", "", "ERIC", "OT" },
    { "", "HERM", "", "Ot}rM" },
    { "", "H", "ER", "ud" },
    { "!!", "H", "+", "Od" },
    { "", "", "ION", "Os" },
    { "", "", "OP", "OT" },
    { "", "OR", "", "OTx|" },
    { ":", "", "AT+", "OT" },
    { ":", "", "%", "OT" },
};
static const fw_rule_list l_ot = { r_ot, 9 };

/* lts_rules_o at LOW $5111: OU */
static const fw_rule r_ou[] = {
    { "", "", "GHT", "o" },
    { "", "GH", "", "uF" },
    { " ", "R", "", "wx|" },
    { "", "TR", "AG", "wT3" },
    { " ", "", "", "w" },
    { "", "", "D", "w" },
    { "", "L", "D", "v" },
    { "", "N", "T", "wN" },
    { "!", "", "!L", "u" },
    { "", "", "P", "U" },
    { ":!#:!", "R", "", "r|" },
    { "", "R", "", "q|" },
    { " :", "", "S#", "w" },
    { "#:", "", "S#", "w" },
    { "", "", "ST", "w" },
    { "", "S", "", "xS" },
    { "", "", "!#", "U" },
    { "", "", "", "w" },
};
static const fw_rule_list l_ou = { r_ou, 18 };

/* lts_rules_o at LOW $517A: OV */
static const fw_rule r_ov[] = {
    { "", "A", "L", "OVx" },
    { "", "", "ER", "OV" },
    { "", "IE", "T", "OVJ" },
    { "#:", "", " ", "QF" },
    { "", "INE", "", "OVIN" },
    { "", "", "", "uV" },
};
static const fw_rule_list l_ov = { r_ov, 6 };

/* lts_rules_o at LOW $51A4: OW */
static const fw_rule r_ow[] = {
    { " :", "", "L", "w" },
    { "S", "", "N", "O" },
    { "ND", "", "N", "O" },
    { "HR", "", "N", "O" },
    { "!", "", "N", "w" },
    { " ", "", "", "O" },
    { "!", "", "ER", "w" },
    { "#:!", "", "% ", "O" },
    { "#:!", "", " ", "O" },
    { "", "", "", "w" },
};
static const fw_rule_list l_ow = { r_ow, 10 };

/* lts_rules_o at LOW $51DD: OX */
static const fw_rule r_ox[] = {
    { "", "FOR", "D", "QkSFx" },
};
static const fw_rule_list l_ox = { r_ox, 1 };

/* lts_rules_o at LOW $51E9: OY */
static const fw_rule r_oy[] = {
    { "", "", "", "y" },
};
static const fw_rule_list l_oy = { r_oy, 1 };

static const fw_rule_list *const k_lts_rules_o[] = {
    &l_oa, &l_ob, &l_oc, &l_od, &l_oe, &l_of,
    &l_og, &l_oh, &l_oi, &l_ok, &l_ol, &l_om,
    &l_on, &l_oo, &l_op, &l_or, &l_os, &l_ot,
    &l_ou, &l_ov, &l_ow, &l_ox, &l_oy,
};
const fw_rule_table lts_rules_o = { "lts_rules_o", 0x4B42, 1707, true, k_lts_rules_o, 23 };

const fw_rule_list *const lts_pairs_o[26] = {
    ['A' - 'A'] = &l_oa,
    ['B' - 'A'] = &l_ob,
    ['C' - 'A'] = &l_oc,
    ['D' - 'A'] = &l_od,
    ['E' - 'A'] = &l_oe,
    ['F' - 'A'] = &l_of,
    ['G' - 'A'] = &l_og,
    ['H' - 'A'] = &l_oh,
    ['I' - 'A'] = &l_oi,
    ['K' - 'A'] = &l_ok,
    ['L' - 'A'] = &l_ol,
    ['M' - 'A'] = &l_om,
    ['N' - 'A'] = &l_on,
    ['O' - 'A'] = &l_oo,
    ['P' - 'A'] = &l_op,
    ['R' - 'A'] = &l_or,
    ['S' - 'A'] = &l_os,
    ['T' - 'A'] = &l_ot,
    ['U' - 'A'] = &l_ou,
    ['V' - 'A'] = &l_ov,
    ['W' - 'A'] = &l_ow,
    ['X' - 'A'] = &l_ox,
    ['Y' - 'A'] = &l_oy,
};

/* lts_rules_p at LOW $51ED: PA */
static const fw_rule r_pa[] = {
    { "", "GEAN", "T", "PajxN" },
    { " ", "LACE", "", "PaLiS" },
    { "", "L", "ETT", "PaL" },
    { "", "LAE", "O", "PaLb" },
    { "", "LE", "O", "PaLb" },
    { " ", "NAM", "AN", "PaNxM" },
    { " ", "NAMA", " ", "PaNxMh" },
    { " ", "NE", "L", "PaNx" },
    { "", "NTS", "", "PaNTS" },
    { "", "P", "ER", "PAP" },
    { " ", "PU", "A", "Papv" },
    { " ", "RAGUAY", "", "PaRxGWI" },
    { "", "RAME", "TER", "PxRaMi" },
    { "", "RAME", "TR", "PaRxMe" },
    { " ", "RA", "", "PaRx" },
    { "", "R", "ABLE", "PR" },
    { " ", "REN", " ", "PxReN" },
    { "", "R", "% ", "Pl|" },
    { " ", "RIS", " ", "PaRiS" },
    { "", "RISHION", "ER", "PaRisxN" },
    { "", "ROCHIA", "L", "PxROCbx" },
    { "", "RTICU", "", "PxTiCYU" },
    { "", "SS", "AGE", "PaS" },
    { "", "SS", "ENG", "PaS" },
    { "", "SS", "ION", "Pas" },
    { "", "SS", "IV+", "PaS" },
    { "", "SS", "", "PhS" },
    { "", "", "STA", "Pa" },
    { "", "S", "T#L", "PaS" },
    { "", "S", "TIM", "PhS" },
    { "", "ST", ":+", "PASf" },
    { "", "TH", "Y ", "Pxt" },
    { "", "TH", "IE", "Pxt" },
    { "", "TH", "ISE", "Pxt" },
    { "", "TH", "ETIC", "Pxt" },
    { " ", "TH", "#", "PAd" },
    { " ", "", "TH:", "Ph" },
    { " ", "TIO", ": ", "PaTbO" },
    { "", "TR", "ON: ", "PAT3" },
    { "", "VE", "", "PAV" },
};
static const fw_rule_list l_pa = { r_pa, 40 };

/* lts_rules_p at LOW $539C: PE */
static const fw_rule r_pe[] = {
    { "", "ASA", "NT", "PeZx" },
    { "", "NA", "LT", "PeNx" },
    { "", "NA", "L", "PENx" },
    { "", "NE", "TR", "PeNx" },
    { "", "OP", "", "PEP" },
    { " ", "RCE", "NT", "PxSe" },
    { "", "R", "FORM", "Px" },
    { " ", "RHAPS", " ", "PxHapS" },
    { "", "RI", "OD", "PJRb" },
    { "", "RI", "MET", "PxRi" },
    { "", "RI", "PH", "PxRi" },
    { " ", "RI", "", "PeRi" },
    { "", "RSEVER", "", "PrSxVJ|" },
    { "", "RSON", "", "PrSxN" },
    { " ", "T", "AL", "PeT" },
    { " ", "TITE", " ", "PxTET" },
    { "", "TI", "TION", "PxTi" },
};
static const fw_rule_list l_pe = { r_pe, 17 };

/* lts_rules_p at LOW $5454: PG */
static const fw_rule r_pg[] = {
    { " ", "UP", " ", "PEjE YUPE" },
};
static const fw_rule_list l_pg = { r_pg, 1 };

/* lts_rules_p at LOW $5464: PH */
static const fw_rule r_ph[] = {
    { "", "ERE", "", "FJ|" },
    { "", "E", "RO", "FJ" },
    { "", "ILIPP", "IN", "FiLiP" },
    { "", "IL", "O", "FiL" },
    { "", "OEN", "IX", "FEN" },
    { "", "ONET", "ICIAN", "FONxT" },
    { "", "ON", "ETIC", "FxN" },
    { "", "ON", "%", "FON" },
    { "", "ON", "O", "FQN" },
    { "", "OT", "OGRAPHY", "FxT" },
    { "", "OT", "OGRAPHER", "FxT" },
    { "", "OTON", "", "FOTQN" },
    { "", "O", "T", "FO" },
    { "", "RAS", "+", "FRAZ" },
    { "", "TH", "", "Ft" },
    { "", "YSIO", "", "FiZbO" },
    { "", "", "", "F" },
};
static const fw_rule_list l_ph = { r_ph, 17 };

/* lts_rules_p at LOW $550B: PI */
static const fw_rule r_pi[] = {
    { " ", "ANO", "", "PYaNO" },
    { " ", "GEON", "", "PijiN" },
    { "", "", "NT", "PI" },
    { "", "ON", "EER", "PIxN" },
    { " ", "", "OUS", "PI" },
    { "", "", "PE", "PI" },
    { "", "LO", "T", "PILx" },
};
static const fw_rule_list l_pi = { r_pi, 7 };

/* lts_rules_p at LOW $554B: PL */
static const fw_rule r_pl[] = {
    { "", "AC", "E", "PLAS" },
    { "", "ANE", "T", "PLaNx" },
    { "", "ANE", "", "PLAN" },
    { "", "A", "NT", "PLh" },
    { "", "AZA", "", "PLhZx" },
    { "", "EASE", "", "PLEZ" },
    { "", "EASA", "N", "PLeZ" },
    { "", "E", "MENT", "PLx" },
    { "", "E", "X", "PLe" },
    { "", "IA", "!!", "PLIx" },
    { "", "IER", "", "PLIx|" },
    { "", "IE", "!", "PLI" },
    { "X", "O", "!+", "pLO" },
    { "", "O", "!+", "PLO" },
    { " IM", "Y", "", "PLI" },
    { "#P", "Y", "", "PLI" },
    { "#", "Y", "", "PLI" },
};
static const fw_rule_list l_pl = { r_pl, 17 };

/* lts_rules_p at LOW $55E6: PN */
static const fw_rule r_pn[] = {
    { " ", "", "", "N" },
};
static const fw_rule_list l_pn = { r_pn, 1 };

/* lts_rules_p at LOW $55EB: PO */
static const fw_rule r_po[] = {
    { " ", "", " BOX", "PEO" },
    { "", "", "CK", "PQ" },
    { "", "", "D: ", "PQ" },
    { "", "", "ETIC", "PO" },
    { "", "E", "M", "POi" },
    { "", "ET", "", "POiT" },
    { "", "L", "AR", "POL" },
    { "", "LICE", "", "PxLES" },
    { "", "LISH", "+", "PQLis" },
    { "", "LI", "TE", "PxLI" },
    { "", "L", "ITICAL", "PxL" },
    { "", "LIT", "IC", "PQLxT" },
    { "", "LI", "!#: ", "PQLx" },
    { " ", "L", "Y", "PQL" },
    { "", "", "ND", "PQ" },
    { "", "N", "ENT", "PON" },
    { " ", "NTE", "!:#!", "PQNTi" },
    { "", "R", ":ATION", "Px|" },
    { " ", "R", "", "Po|" },
    { "", "S", "ED", "POZ" },
    { "", "S", "%", "POZ" },
    { "", "SI", "TION", "PxZi" },
    { "", "SI", "TE", "PxZi" },
    { "", "SI", "", "PQZx" },
    { "", "SSE", " ", "PQSb" },
    { "", "SS", "IB", "PQS" },
    { "", "SS", "", "PxS" },
    { " ", "", "ST", "PO" },
    { "", "TEN", "TI#", "PxTeN" },
    { "", "T", "EN", "POT" },
    { "", "T", "!:#", "PQT" },
    { "", "ULTRY", "", "PO1T3b" },
    { "", "VER", "", "PQVr|" },
    { "", "W", "", "Pw" },
    { " ", "", "!:#!:ATION", "PQ" },
    { " ", "", "!:U:", "PQ" },
    { " ", "", "!:#!:#", "Px" },
};
static const fw_rule_list l_po = { r_po, 37 };

/* lts_rules_p at LOW $574B: PP */
static const fw_rule r_pp[] = {
    { "", "LY", "", "PLI" },
    { "", "AR", "", "Pa|" },
};
static const fw_rule_list l_pp = { r_pp, 2 };

/* lts_rules_p at LOW $575A: PR */
static const fw_rule r_pr[] = {
    { "", "ACTISE", "", "PRakTiS" },
    { "", "E", "CIAT+", "PRE" },
    { "", "EDE", "CE", "PREDb" },
    { "", "E", "EMPT", "PRE" },
    { "", "E", "FER#", "PRe" },
    { "", "E", "FER", "PRx" },
    { "", "E", "FIX", "PRE" },
    { "", "E", "MIUM", "PRE" },
    { "", "EPAI", "D", "PREPA" },
    { "", "E", "PARE", "PRi" },
    { "", "ETT", "Y", "PRiT" },
    { "", "ES", "ENT", "PReZ" },
    { "", "ES", "ID+", "PReZ" },
    { "", "ESTIGE", "", "PReSfEj" },
    { "", "EVE", "NT", "PRiVe" },
    { "", "E", "VIEW", "PRE" },
    { "", "EV", "IOUS", "PREV" },
    { " ", "E", " ", "PRE" },
    { "", "IA", "T+", "PRJ" },
    { " ", "IC", "+", "PRIS" },
    { "", "IMARI", "LY", "PRIMRx" },
    { " ", "IM", "IT", "PRiM" },
    { " ", "IM", "#", "PRIM" },
    { "", "IOR", "I", "PRIQR" },
    { " ", "IOR", " ", "PRIx" },
    { "", "ISON", "", "PRiZ7" },
    { " ", "IVA", "T", "PRIVx" },
    { "", "IVI", "L", "PRiVi" },
    { "", "OB", "AB", "PRQB" },
    { "", "OBLE", "M", "PRQBLx" },
    { " ", "OCEDUR", "", "PRxSEjx|" },
    { " ", "OCE", "D", "PRxSE" },
    { "", "OCESS", "", "PROSeS" },
    { " ", "OCOMM", " ", "PROKQM" },
    { " ", "ODU", "C+", "PRxjU" },
    { " ", "ODUC", "T:", "PRQDuK" },
    { "", "O", "FESSION", "PRx" },
    { "", "O", "FESS", "PRQ" },
    { "", "OFI", "T", "PRQFi" },
    { "", "O", "GR", "PRO" },
    { " ", "OMIN", "", "PRQMiN" },
    { "", "OMIS", "+", "PRQMiS" },
    { "", "OMENADE", "", "PRQMxNhD" },
    { " ", "OM", "", "PRQM" },
    { " ", "OMI", "", "PRQMi" },
    { "", "ONOU", "NC", "PRxNw" },
    { "", "O", "NOUN", "PRO" },
    { "", "O", "NUN", "PRx" },
    { "", "OP", " ", "PRQp" },
    { "", "OPA", "GAT+", "PRQPx" },
    { "", "OPHE", "&+", "PRQFx" },
    { "", "OPOS", "", "PRxPOZ" },
    { "", "OPRIE", "T", "PRQPRIx" },
    { "", "O", "P#R", "PRQ" },
    { "", "O", "SP", "PRQ" },
    { "", "OSE", "CU", "PRQSi" },
    { "", "OS", "ODIC", "PRxZ" },
    { "", "OSO", "DY", "PRQZx" },
    { "", "OTEC", "T", "PRxTek" },
    { "", "O", "TESTANT", "PRQ" },
    { " ", "OTOCO", "L", "PROTxKQ" },
    { "", "OTO", "!#", "PROTO" },
    { "", "O", "VERB", "PRQ" },
    { "", "OV", "%", "PRUV" },
    { " ", "O", "VIDEN", "PRQ" },
    { " ", "O", "VID", "PRx" },
    { " ", "O", "V", "PRQ" },
    { "", "OW", "", "PRw" },
    { "", "OX", "", "PRQkS" },
    { " ", "O", "!#", "PRO" },
};
static const fw_rule_list l_pr = { r_pr, 70 };

/* lts_rules_p at LOW $5A78: PS */
static const fw_rule r_ps[] = {
    { "", "YCHE", " ", "SIKE" },
    { "", "YCHE", "!:#", "SIKx" },
    { "", "YCHO", "", "SIKO" },
    { "", "YCH", "", "SIK" },
    { "", "YEUD", "O", "SYUD" },
    { " ", "I", " ", "pSI" },
    { " ", "Y", "", "SI" },
    { " ", "", "", "S" },
};
static const fw_rule_list l_ps = { r_ps, 8 };

/* lts_rules_p at LOW $5AC0: PT */
static const fw_rule r_pt[] = {
    { " ", "", "", "T" },
};
static const fw_rule_list l_pt = { r_pt, 1 };

/* lts_rules_p at LOW $5AC5: PU */
static const fw_rule r_pu[] = {
    { "", "", "LL", "Pv" },
    { "", "NISH", "", "PuNis" },
    { "", "RE", "", "PYq|" },
    { " ", "RPOS", "E", "PrPxS" },
    { "", "SH", "", "Pvs" },
    { "", "T", "%", "PYUT" },
    { "", "", "TT", "Pu" },
    { "", "", "T", "Pv" },
};
static const fw_rule_list l_pu = { r_pu, 8 };

/* lts_rules_p at LOW $5B06: PY */
static const fw_rule r_py[] = {
    { "", "", "JAMA", "Pi" },
    { "", "", "RAM", "Pi" },
    { "", "", "THON", "PI" },
};
static const fw_rule_list l_py = { r_py, 3 };

static const fw_rule_list *const k_lts_rules_p[] = {
    &l_pa, &l_pe, &l_pg, &l_ph, &l_pi, &l_pl,
    &l_pn, &l_po, &l_pp, &l_pr, &l_ps, &l_pt,
    &l_pu, &l_py,
};
const fw_rule_table lts_rules_p = { "lts_rules_p", 0x51ED, 2353, true, k_lts_rules_p, 14 };

const fw_rule_list *const lts_pairs_p[26] = {
    ['A' - 'A'] = &l_pa,
    ['E' - 'A'] = &l_pe,
    ['G' - 'A'] = &l_pg,
    ['H' - 'A'] = &l_ph,
    ['I' - 'A'] = &l_pi,
    ['L' - 'A'] = &l_pl,
    ['N' - 'A'] = &l_pn,
    ['O' - 'A'] = &l_po,
    ['P' - 'A'] = &l_pp,
    ['R' - 'A'] = &l_pr,
    ['S' - 'A'] = &l_ps,
    ['T' - 'A'] = &l_pt,
    ['U' - 'A'] = &l_pu,
    ['Y' - 'A'] = &l_py,
};

/* lts_rules_q at LOW $5B1E: QU */
static const fw_rule r_qu[] = {
    { "", "A", "CK", "6Wa" },
    { "", "AI", "", "6WA" },
    { "", "AK", "+", "6WAK" },
    { "", "AL", "M", "6Wh" },
    { "", "AR", "R", "6WQ|" },
    { "", "AR", "!", "6Wo|" },
    { "", "AY", "", "6E" },
    { "", "ASI", "", "6WAZI" },
    { "", "A", "V", "6WA" },
    { "", "A", "", "6WQ" },
    { " ", "EBECOIS", "", "KxBekWh" },
    { " ", "EBEC", " ", "6WxBek" },
    { "", "ER", "Y", "6WJR" },
    { "", "E", "ST", "6We" },
    { "", "ET", "", "6A" },
    { "", "EUE", "", "6YU" },
    { ":#:", "E", " ", "k" },
    { "", "IV", "ER", "6WiV" },
    { "", "I", "P", "6Wi" },
    { " ", "ITE", " ", "6WIT" },
    { "", "IE", "SC", "6Wbe" },
    { "", "IE", "T", "6WIx" },
    { "", "", "", "6W" },
};
static const fw_rule_list l_qu = { r_qu, 23 };

static const fw_rule_list *const k_lts_rules_q[] = {
    &l_qu,
};
const fw_rule_table lts_rules_q = { "lts_rules_q", 0x5B1E, 201, true, k_lts_rules_q, 1 };

const fw_rule_list *const lts_pairs_q[26] = {
    ['U' - 'A'] = &l_qu,
};

/* lts_rules_r at LOW $5BE7: RA */
static const fw_rule r_ra[] = {
    { "", "DAR", "", "RADh|" },
    { "", "DIO", "", "RADbO" },
    { "", "MPAG", "+", "RaMPAj" },
    { ":", "N", "CH", "RhN" },
    { "", "NG", "+", "RANj" },
    { " ", "PI", "D", "RaPi" },
    { "", "R", "Y", "RxR" },
    { "", "R", "IES", "RxR" },
    { " ", "", "TION", "Ra" },
    { "", "VE", "LY", "RAV" },
    { "", "VE", "L", "RaVx" },
};
static const fw_rule_list l_ra = { r_ra, 11 };

/* lts_rules_r at LOW $5C50: RE */
static const fw_rule r_re[] = {
    { "", "ACH", "", "REc" },
    { "", "AC", "T", "RbaK" },
    { " ", "ADME", " ", "REDME" },
    { "", "A", "DY", "Re" },
    { " ", "A", "D% ", "RE" },
    { " ", "ADI", "!:#", "ReDi" },
    { " ", "A", "D", "RE" },
    { " ", "", "ALI", "RE" },
    { "", "AL", "M", "Re1" },
    { "!", "ASE", "", "RES" },
    { "", "A", "SON", "RE" },
    { "", "A", "SS", "RJ" },
    { " ", "BE", "L: ", "ReBx" },
    { " ", "BEL", "", "RxBeL" },
    { " ", "B", "+", "ReB" },
    { "", "", "CAP", "RE" },
    { "", "CE", "NT", "RESx" },
    { "", "CIPE", "", "ReSiPE" },
    { "", "CIT", "A", "ReSIT" },
    { "", "COGNIS", "#", "ReKxGNIZ" },
    { "", "COG", "N", "ReKxG" },
    { " ", "", "COLL", "Re" },
    { " ", "COMM", "", "Re6QM" },
    { "", "", "CORD%", "Ri" },
    { "", "", "CORDED", "Ri" },
    { "", "", "CORD", "Re" },
    { "", "", "COVER", "Ri" },
    { "", "CRUI", "T", "RiKRU" },
    { "", "CT", "", "ReKT" },
    { " ", "", "DR", "Ri" },
    { " ", "", "D#", "Rb" },
    { " ", "", "D:", "Re" },
    { " ", "FER", ": ", "RbF}r|" },
    { " ", "FER", "E", "ReFx|" },
    { " ", "", "FUGE", "Re" },
    { "", "GIME", " ", "RAz}EM" },
    { " ", "GIO", "N", "REjx" },
    { " ", "G", "ULAR", "ReG" },
    { " ", "G", "+", "Rej" },
    { "", "HEARSE", "", "RiH}rS" },
    { "", "IGN", "", "RAN" },
    { "", "IN", "% ", "RAN" },
    { "", "IN", " ", "RAN" },
    { " ", "LAT", "IV", "ReLxT" },
    { " ", "LEAS", "", "RiLES" },
    { "", "LEV", "AN", "ReLxV" },
    { "", "", "LIC", "Re" },
    { "", "", "LISH", "Re" },
    { "", "LIEF", "", "RiLEF" },
    { "", "LI", "#!", "RiLI" },
    { "", "LY", "", "RiLI" },
    { "", "M", "EDY", "ReM" },
    { "", "M", "NAN", "ReM" },
    { "", "", "ND", "Re" },
    { "", "", "NAME", "Rb" },
    { " ", "NT", "", "ReNT" },
    { " ", "N", " ", "ReN" },
    { "", "", "PARAB", "Re" },
    { "", "", "PLICA", "Re" },
    { "", "PRE", "!#", "RePRi" },
    { "", "PRI", "!#", "RePRi" },
    { "", "P", "T", "ReP" },
    { "", "", "PUTA", "Re" },
    { "", "QUIS", "", "Re6WiZ" },
    { "", "SCI", "ND", "RESi" },
    { "", "SCUE", "", "ReSCYU" },
    { "", "S", "ERVAT", "ReZ" },
    { "", "", "SET", "RE" },
    { "", "S", "IDENT", "ReZ" },
    { "", "S", "ID+", "RiZ" },
    { "", "SIG", "NA", "ReZiG" },
    { "", "S", "IGN", "RiZ" },
    { "", "", "SIN", "Re" },
    { "", "SOLU", "", "ReZxLU" },
    { "", "SON", "", "ReZxN" },
    { "", "SPON", "", "RiSpQN" },
    { " ", "", "ST ", "Re" },
    { " ", "STAURA", "NT", "ReSTxRQ" },
    { "", "", "STR", "Ri" },
    { "", "TCH", "", "Rec" },
    { "", "TIN", "", "ReTiN" },
    { "", "TRO", "", "ReT3O" },
    { " ", "", "TRY", "RE" },
    { " ", "", "TRI", "RE" },
    { " ", "", "TURN", "Ri" },
    { "", "VE", "L", "ReVx" },
    { "", "VENUE", "", "ReVxNYU" },
    { "", "VER", "EN", "ReVx|" },
    { "", "VERE", "", "RiVJ|" },
    { "", "VIS", "E", "RiVIZ" },
    { "", "", "VIV", "Ri" },
    { "", "V", "OLU", "ReV" },
    { "", "ZIN", "", "ReZiN" },
    { " ", "", " ", "RE" },
    { "", "", " ", "x|" },
    { "!", "S", " ", "xZ" },
    { " ", "", "#", "RE" },
    { " ", "", "", "Ri" },
};
static const fw_rule_list l_re = { r_re, 98 };

/* lts_rules_r at LOW $5FFD: RH */
static const fw_rule r_rh[] = {
    { "", "U", "", "RU" },
    { "", "", "", "R" },
};
static const fw_rule_list l_rh = { r_rh, 2 };

/* lts_rules_r at LOW $6006: RI */
static const fw_rule r_ri[] = {
    { "", "CE", "", "RIS" },
    { "", "DE", "", "RID" },
    { "", "PE", "", "RIP" },
    { "", "SEN", "", "RiZxN" },
    { "", "O", "T", "RIx" },
    { "", "T", "UAL", "Ric" },
    { "", "VA", "L", "RIVx" },
    { " ", "V", "ER", "RiV" },
};
static const fw_rule_list l_ri = { r_ri, 8 };

/* lts_rules_r at LOW $6048: RM */
static const fw_rule r_rm[] = {
    { " ", "DIR", " ", "hReM DEIh|" },
};
static const fw_rule_list l_rm = { r_rm, 1 };

/* lts_rules_r at LOW $605A: RN */
static const fw_rule r_rn[] = {
    { "", "IB", "", "hReNIBE" },
};
static const fw_rule_list l_rn = { r_rn, 1 };

/* lts_rules_r at LOW $6066: RO */
static const fw_rule r_ro[] = {
    { "", "", "BOT", "RO" },
    { "", "", "BUST", "RO" },
    { "", "", "MANIA", "RU" },
    { "", "MAN", "", "ROMxN" },
    { " ", "W", "", "RO" },
};
static const fw_rule_list l_ro = { r_ro, 5 };

/* lts_rules_r at LOW $608F: RU */
static const fw_rule r_ru[] = {
    { "", "GGE", "D", "RuGi" },
    { "", "IN", "", "RUiN" },
    { "", "IS", "%", "RUZ" },
    { "", "", "MANIA", "RU" },
    { "", "N", "ER", "RuN" },
    { "", "NE", ": ", "RUN" },
    { "", "N", "%", "RuN" },
    { "", "", "RAL", "RU" },
    { "", "SS", "IA", "Rus" },
};
static const fw_rule_list l_ru = { r_ru, 9 };

static const fw_rule_list *const k_lts_rules_r[] = {
    &l_ra, &l_re, &l_rh, &l_ri, &l_rm, &l_rn,
    &l_ro, &l_ru,
};
const fw_rule_table lts_rules_r = { "lts_rules_r", 0x5BE7, 1268, true, k_lts_rules_r, 8 };

const fw_rule_list *const lts_pairs_r[26] = {
    ['A' - 'A'] = &l_ra,
    ['E' - 'A'] = &l_re,
    ['H' - 'A'] = &l_rh,
    ['I' - 'A'] = &l_ri,
    ['M' - 'A'] = &l_rm,
    ['N' - 'A'] = &l_rn,
    ['O' - 'A'] = &l_ro,
    ['U' - 'A'] = &l_ru,
};

/* lts_rules_s at LOW $60DB: SA */
static const fw_rule r_sa[] = {
    { "", "CRE", "D", "SAKRi" },
    { "", "CRIFICE", "", "SaKRxFIS" },
    { " ", "FE", "", "SAF" },
    { " ", "GA", ": ", "ShGx" },
    { "", "I", "D", "Se" },
    { "", "", "LE: ", "SA" },
    { "", "", "LINE", "SA" },
    { "", "", "LINI", "Sx" },
    { "", "LIVA", "", "SxLIVx" },
    { " ", "LMON", "", "SaMxN" },
    { "", "LU", "T+", "SxLU" },
    { " ", "MO", "A", "SxMO" },
    { "", "TELLITE", "", "SaTxLIT" },
    { "", "TISFY", "", "SaTiSFI" },
    { " ", "TUR", "DAY", "SaTx" },
    { "", "", "VOU", "SA" },
    { " ", "UDI", "", "SaUDb" },
    { "", "Y", " ", "SA" },
};
static const fw_rule_list l_sa = { r_sa, 18 };

/* lts_rules_s at LOW $6198: SC */
static const fw_rule r_sc[] = {
    { "", "A", "L#", "SkA" },
    { "", "AR", "C+", "Skl" },
    { "", "EN", "IC", "SEN" },
    { "", "HEDU", "LE", "seDYU" },
    { "", "HIZOPH", "", "SkiTSxF" },
    { " ", "H", "", "Sk" },
    { " ", "I", "#", "SI" },
    { " ", "ORE", "", "Sko|" },
    { "", "OT", "LAN", "SkQT" },
    { "", "REE", "CH", "SkRE" },
};
static const fw_rule_list l_sc = { r_sc, 10 };

/* lts_rules_s at LOW $61FC: SE */
static const fw_rule r_se[] = {
    { "", "AR", "CH", "Sr" },
    { "", "C", "ANT", "SEK" },
    { "", "CRETA", "R", "SeKRxTl" },
    { "", "CRET", "IV", "SEKRxT" },
    { "", "CRE", "T+", "SiKRE" },
    { "", "CRE", "T", "SEKRx" },
    { "", "CREC", "Y", "SEKRxS" },
    { "", "", "CURE", "Si" },
    { "#", "", "D ", "Z" },
    { "#", "EM", "", "SEM" },
    { "", "I", "SM#", "SI" },
    { "", "IZ", "URE", "SEz" },
    { "", "IZ", "+", "SEZ" },
    { "", "L", "F", "Se1" },
    { "", "L", "V", "Se1" },
    { "", "MI", "", "SeMi" },
    { " ", "NA", "TE", "SeNx" },
    { " ", "NEGAL", "ESE", "SeNiGxL" },
    { " ", "NEGA", "L", "SeNiGo" },
    { "", "", "NTIA", "Se" },
    { "", "", "NT", "Se" },
    { "", "NIOR", "", "SENJ|" },
    { "", "PAR", "ATE", "SePxR" },
    { "", "PARA", "TI", "SePxRA" },
    { " ", "", "QUENC+", "SE" },
    { "", "", "QUEN", "Si" },
    { " ", "R", "#", "SJR" },
    { "", "RGEA", "NT", "Shjx" },
    { "", "RVIC", "#", "SrViS" },
    { "", "SAME", "", "SeSxMb" },
    { "#", "T", "", "SeT" },
    { "", "VERE", "", "SiVJ|" },
    { "", "W", "ER", "SU" },
    { "", "W", "AGE", "SU" },
    { " ", "W", "", "SO" },
    { "", "X", "UAL", "Seks" },
    { " ", "YCH", "ELLES", "SAs" },
    { " ", "YCHELLOIS", "", "SAseLWh" },
    { "#:!", "Y", " ", "Zb" },
};
static const fw_rule_list l_se = { r_se, 39 };

/* lts_rules_s at LOW $638E: SH */
static const fw_rule r_sh[] = {
    { "", "ADOW", "", "saDO" },
    { " ", "A", "LL", "sa" },
    { " ", "AR", "+", "sl|" },
    { "", "EPH", "ERD", "seP" },
    { " ", "E", " ", "~sE" },
    { " ", "E", "'", "~sE" },
    { "", "INE", "", "sIN" },
    { " ", "IRE", ": ", "sIx" },
    { "", "IRE", " ", "sx|" },
    { "", "", "ION", "s" },
    { "", "IV", "ER", "siV" },
    { "", "OE", "", "sU" },
    { "", "ONE", "", "sQN" },
    { "", "ORT", "", "soT" },
    { " ", "OR", "+", "so|" },
    { "", "OULD", "ER", "sO1D" },
    { "", "OU", "T", "sw" },
    { "", "OW", "ER", "sw" },
    { "", "OW", "", "sO" },
    { "", "", "", "s" },
};
static const fw_rule_list l_sh = { r_sh, 20 };

/* lts_rules_s at LOW $6433: SI */
static const fw_rule r_si[] = {
    { "", "A", "L", "sx" },
    { "#", "D", "+", "SID" },
    { "", "E", "G+", "SE" },
    { " ", "ERRA", " ", "SElRx" },
    { "#", "ER", "", "ZJ|" },
    { "", "EV", "+", "ZiV" },
    { "", "GH", "T", "SI" },
    { "", "GN", "A", "SiGN" },
    { "", "GN", "IF", "SiGN" },
    { " ", "L", "ENC", "SIL" },
    { "", "MU", "LTAN", "SiMx" },
    { " ", "NC", "ER+", "SiNS" },
    { " ", "NCE", " ", "SiNS" },
    { "", "NG", "L", "SinG" },
    { "", "NEW", "", "SiNYU" },
    { "#", "O", "N", "zx" },
    { "#R", "O", "N", "zx" },
    { "!", "O", "N", "sx" },
    { "", "PH", "ON", "SIF" },
    { "", "S", "", "SiS" },
    { "", "", "TE", "SI" },
    { " ", "TU", "", "SicU" },
    { "", "", "T", "Si" },
    { "#:", "ES", " ", "ZEZ" },
    { "#", "", "#", "z" },
    { "R", "A", ": ", "zx" },
    { "R", "", "#", "z" },
    { "L", "", "#", "s" },
};
static const fw_rule_list l_si = { r_si, 28 };

/* lts_rules_s at LOW $651A: SK */
static const fw_rule r_sk[] = {
    { " ", "I", " ", "SkE" },
    { " ", "I", "% ", "SkE" },
    { "", "", "", "Sk" },
};
static const fw_rule_list l_sk = { r_sk, 3 };

/* lts_rules_s at LOW $6530: SL */
static const fw_rule r_sl[] = {
    { "", "OVAK", "", "SLOVaK" },
    { "", "OVEN", "#", "SLOVEN" },
    { "", "OW", "", "SLO" },
    { "", "", "", "SL" },
};
static const fw_rule_list l_sl = { r_sl, 4 };

/* lts_rules_s at LOW $6555: SM */
static const fw_rule r_sm[] = {
    { "", "", "#", "SM" },
    { "#", "", " ", "ZM" },
};
static const fw_rule_list l_sm = { r_sm, 2 };

/* lts_rules_s at LOW $6561: SN */
static const fw_rule r_sn[] = {
    { " ", "OW", "", "SNO" },
    { "", "", "#", "SN" },
    { "#", "", "'", "ZxN" },
};
static const fw_rule_list l_sn = { r_sn, 3 };

/* lts_rules_s at LOW $6576: SO */
static const fw_rule r_so[] = {
    { " ", "C", "IET", "SxS" },
    { "", "", "CK", "SQ" },
    { " ", "F", "A: ", "SOF" },
    { "", "", "FT", "SQ" },
    { "", "L", "AR", "SOL" },
    { "", "LO", "", "SOLO" },
    { "", "L", "EMNI", "SxL" },
    { "", "L", "EMN", "SQL" },
    { "", "LUB", "LE: ", "SQLYvB" },
    { "#", "LU", "T+", "SxLU" },
    { "", "MAL", "IA", "SxMhL" },
    { "#:", "ME", "", "SxM" },
    { "", "ME", "", "SuM" },
    { " ", "", "PHI", "Sx" },
    { "", "RROW", "", "SQRO" },
    { "", "RR", "", "SQ|" },
    { "#", "", "T:", "SQ" },
    { " ", "U", "L", "SO" },
    { "", "URC", "+", "SoS" },
    { "", "UR", "", "Swx|" },
    { " ", "UTH", "ER", "Sud" },
    { "", "UVEN", "IR", "SUVxN" },
    { "", "VEREIGN", "", "SQVRiN" },
    { " ", "W", "", "SO" },
};
static const fw_rule_list l_so = { r_so, 24 };

/* lts_rules_s at LOW $6655: SP */
static const fw_rule r_sp[] = {
    { "", "ACEBAR", "", "SpASBh|" },
    { "", "EC", "IES ", "SpEs" },
    { "", "EC", "IFICAT", "SpeS" },
    { "", "EC", "IFIC", "SPxS" },
    { "", "EC", "IF", "SpeS" },
    { "", "HERE", "", "SFJ|" },
    { "", "H", "ERIC", "SF" },
    { "", "I", "NDL", "Spi" },
    { "", "IRIT", "UAL", "SpiRic" },
    { "", "IRI", "T", "SpiRi" },
    { "", "OKE", "", "SpOK" },
    { "", "ONG", "+", "SpuNj" },
    { " ", "", "", "Sp" },
    { "", "", "#", "Sp" },
};
static const fw_rule_list l_sp = { r_sp, 14 };

/* lts_rules_s at LOW $66E9: SQ */
static const fw_rule r_sq[] = {
    { "", "UARE", "", "S6Wl|" },
};
static const fw_rule_list l_sq = { r_sq, 1 };

/* lts_rules_s at LOW $66F5: SS */
static const fw_rule r_ss[] = {
    { "", "ED", " ", "Sf" },
    { "", "ION", "", "sxN" },
    { "", "IER", "", "SJ|" },
    { "", "", "I#", "s" },
    { " !:#", "UR", "E", "sx|" },
    { "#", "U", "#", "sU" },
};
static const fw_rule_list l_ss = { r_ss, 6 };

/* lts_rules_s at LOW $6725: ST */
static const fw_rule r_st[] = {
    { " :#:!#:!", "ANC", "+", "SfaNS" },
    { "", "ANCE", "", "SfhNS" },
    { "", "A", "PLE", "SfA" },
    { "", "AR", " ", "Sfh|" },
    { "", "A", "TE", "SfA" },
    { "", "A", "TION", "SfA" },
    { "", "ATUE", "", "SfacU" },
    { "", "ATUS", "", "SfATxS" },
    { "", "EA", "LTH", "Sfe" },
    { "", "EA", "K", "SfA" },
    { "#", "", "EN", "Sf" },
    { "", "ERE", "O", "SfeRE" },
    { "", "ERI", "LE: ", "SfeRI" },
    { "", "I", "FL", "SfI" },
    { "#", "", "ION", "Sc" },
    { "", "IRRE", "D", "Sfr" },
    { "", "IRR", "UP", "SfiR" },
    { "", "IR", ":", "Sfr|" },
    { "", "I", "", "Sfi" },
    { "", "LESS", "", "SfLxS" },
    { "", "L", "%", "SL" },
    { " ", "OMACH", "", "SfuMxk" },
    { "", "OO", "D", "Sfv" },
    { "", "OR", "AGE", "SfoR" },
    { " ", "OR", "+", "Sfo|" },
    { "", "OVE", "", "SfOV" },
    { "", "RAT", "EGIC", "Sf3xT" },
    { "", "RAT", "EG", "Sf3aT" },
    { "", "RONG", "+", "Sf3QnG" },
    { "", "ROPHE", "", "Sf3xFb" },
    { "", "ROVE", "", "Sf3OV" },
    { " ", "UDE", "NT", "SfYUDx" },
    { "", "UDI", "OUS", "SfYUDb" },
    { "", "UDIO", "", "SfYUDbO" },
    { " ", "UD", "+", "SfuD" },
};
static const fw_rule_list l_st = { r_st, 35 };

/* lts_rules_s at LOW $689C: SU */
static const fw_rule r_su[] = {
    { "", "ADE", "", "SWAD" },
    { "", "A", "SIV", "SWA" },
    { "", "BT", "L+", "SuT" },
    { "", "BUR", "B", "SuBr" },
    { "", "C", "C", "SuK" },
    { " ", "DAN", "", "SUD}aN" },
    { "", "FFICE", "", "SxFIS" },
    { "", "GGE", "ST", "Sxje" },
    { " ", "G", "AR", "svG" },
    { "", "ICID", "", "SUiSID" },
    { " ", "LT", "R", "Su1T" },
    { "#", "M", "%", "ZYUM" },
    { "N", "M", "%", "SYUM" },
    { ".", "M", "%", "ZYUM" },
    { "", "M", "%", "SYUM" },
    { "", "MMON", "", "SuMxN" },
    { " ", "", "N", "Su" },
    { "", "PREME", "", "{SUPREM" },
    { " ", "RE", "T", "sqRx" },
    { " ", "RE", "", "sq|" },
    { "L", "RE", "", "sx|" },
    { "R", "RE", "", "zx|" },
    { "#", "RE", "", "zx|" },
    { " +N", "R", "E", "sq|" },
    { "N", "RE", "", "sx|" },
    { "", "RFA", "C+", "SrFi" },
    { "", "RG", "EON", "Srj" },
    { " ", "RPRIS", "+", "SxPRIZ" },
    { " ", "R", "VIV+", "Sr" },
    { "#", "R", "#", "zr|" },
    { "", "R", "#", "sr|" },
    { "#", "", "#", "zU" },
};
static const fw_rule_list l_su = { r_su, 32 };

/* lts_rules_s at LOW $69CC: SW */
static const fw_rule r_sw[] = {
    { "", "ALLOW", "", "SWQLO" },
    { " ", "A", "M", "SWa" },
    { " ", "A", "P", "SWQ" },
    { "", "EAR", "", "SWl|" },
    { " ", "E", "D#", "SWE" },
    { "", "EE", "T", "SWE" },
    { " ", "I", "NDON", "SWi" },
    { "", "ITZER", "", "SWiTSx" },
    { "", "OR", "D", "So" },
};
static const fw_rule_list l_sw = { r_sw, 9 };

/* lts_rules_s at LOW $6A22: SY */
static const fw_rule r_sy[] = {
    { " ", "MB", "OLIC", "SiMB" },
    { " ", "MBO", "L", "SiMBx" },
    { "", "MM", "ETRIC", "SiM" },
    { "", "MMETR", "Y", "SiMxT3" },
    { "", "NTHESIS", "#", "SiNtxSIZ" },
    { "", "N", "", "SiN" },
    { "", "RING", "+", "SiRiNj" },
    { "", "STEM", "IC", "SiSfEM" },
    { "", "STEM", "", "SiSTxM" },
    { " ", "S", " ", "SiS" },
    { " ", "", "", "Si" },
};
static const fw_rule_list l_sy = { r_sy, 11 };

static const fw_rule_list *const k_lts_rules_s[] = {
    &l_sa, &l_sc, &l_se, &l_sh, &l_si, &l_sk,
    &l_sl, &l_sm, &l_sn, &l_so, &l_sp, &l_sq,
    &l_ss, &l_st, &l_su, &l_sw, &l_sy,
};
const fw_rule_table lts_rules_s = { "lts_rules_s", 0x60DB, 2503, true, k_lts_rules_s, 17 };

const fw_rule_list *const lts_pairs_s[26] = {
    ['A' - 'A'] = &l_sa,
    ['C' - 'A'] = &l_sc,
    ['E' - 'A'] = &l_se,
    ['H' - 'A'] = &l_sh,
    ['I' - 'A'] = &l_si,
    ['K' - 'A'] = &l_sk,
    ['L' - 'A'] = &l_sl,
    ['M' - 'A'] = &l_sm,
    ['N' - 'A'] = &l_sn,
    ['O' - 'A'] = &l_so,
    ['P' - 'A'] = &l_sp,
    ['Q' - 'A'] = &l_sq,
    ['S' - 'A'] = &l_ss,
    ['T' - 'A'] = &l_st,
    ['U' - 'A'] = &l_su,
    ['W' - 'A'] = &l_sw,
    ['Y' - 'A'] = &l_sy,
};

/* lts_rules_t at LOW $6AA2: TA */
static const fw_rule r_ta[] = {
    { "", "BLET", ": ", "TaBLxT" },
    { "!O:", "G", "+", "Thj" },
    { "", "K", "+", "TAK" },
    { "", "LE", "NT", "TaLx" },
    { "", "LLEST", "", "ToLiSf" },
    { "", "NG", "E!:", "TaNj" },
    { " ", "NZAN", "IA", "TaNZxN" },
    { "", "P", "ER", "TAP" },
    { "", "ST", ":+", "TASf" },
};
static const fw_rule_list l_ta = { r_ta, 9 };

/* lts_rules_t at LOW $6B05: TC */
static const fw_rule r_tc[] = {
    { "", "H", "", "c" },
};
static const fw_rule_list l_tc = { r_tc, 1 };

/* lts_rules_t at LOW $6B0A: TE */
static const fw_rule r_te[] = {
    { "", "AR", "", "Tl|" },
    { "", "CHN", "", "TekN" },
    { "", "D", "I#", "TED" },
    { "", "ENAGE", "", "TENAj" },
    { "", "LE", "T", "TeLe" },
    { "", "LE", "", "TeLi" },
    { "", "MPERA", "T", "TeMPRa" },
    { "", "N", "ANCE", "TxN" },
    { " ", "RR", "#", "TeR" },
};
static const fw_rule_list l_te = { r_te, 9 };

/* lts_rules_t at LOW $6B61: TH */
static const fw_rule r_th[] = {
    { " ", "AN", " ", "~dxN" },
    { " ", "ATCH", "", "tac" },
    { " ", "A", "T", "~da" },
    { " ", "E", " #", "~dE" },
    { " ", "E", " ", "~dx" },
    { "#:", "E", " ", "d" },
    { " ", "EATRE", "", "tJTx|" },
    { "", "E", "D ", "d" },
    { "", "EIR", "", "~dl|" },
    { "", "ELE", "SS ", "dxLe" },
    { " ", "EME", "", "tEM" },
    { " ", "EM", "", "~deM" },
    { " ", "EN", "", "~deN" },
    { " ", "E", "O", "tE" },
    { "", "ERA", "", "teRx" },
    { " ", "ERE", "", "~dl|" },
    { "", "ER", ": ", "dx|" },
    { "", "ES", " ", "dZ" },
    { "", "ESE", " ", "dEZ" },
    { " ", "EY", "", "~dA" },
    { " ", "INE", " ", "dIN" },
    { " ", "ISTL", "E", "tiS1" },
    { " ", "IS", "", "diS" },
    { "", "OO", "D", "THv" },
    { " ", "OSE", "", "~dOZ" },
    { "", "OUGH", " ", "~dO" },
    { " ", "REA", "T", "tRe" },
    { "", "ROUGH", "", "tRU" },
    { "", "ROW", "", "tRO" },
    { " ", "US", " ", "~duS" },
    { "", "URS", "DAY", "trZ" },
    { " ", "Y", " ", "dI" },
    { "", "", "Y ", "d" },
    { ":#", "", "ER", "d" },
    { "#", "", "M: ", "dx" },
    { "", "", "", "t" },
};
static const fw_rule_list l_th = { r_th, 36 };

/* lts_rules_t at LOW $6C9D: TI */
static const fw_rule r_ti[] = {
    { "", "", "ALITY", "sb" },
    { "", "", "AL", "s" },
    { "", "A", "N", "sx" },
    { "", "", "ATION", "sb" },
    { "", "", "AT%", "sb" },
    { "", "", "A", "s" },
    { "", "EN", "", "sxN" },
    { "", "G", "ER", "TIG" },
    { "", "LD", "A", "Ti1D" },
    { "", "", "METR", "Ti" },
    { "", "ME", "", "TIM" },
    { " ", "N", "Y", "TIN" },
    { "", "ON", "", "sxN" },
    { "", "O", ": ", "sbO" },
    { "#:", "O", "MET", "sbQ" },
    { "", "", "O", "s" },
    { "", "", "PLY", "Ti" },
    { " ", "", "TAN ", "TI" },
    { "", "T", "L", "TIT" },
    { "", "", "TE", "TI" },
    { "", "", "T", "Ti" },
    { "S", "", "#N", "c" },
};
static const fw_rule_list l_ti = { r_ti, 22 };

/* lts_rules_t at LOW $6D3B: TO */
static const fw_rule r_to[] = {
    { " ", "", " !", "~Tx" },
    { " ", "", " ", "~TU" },
    { " ", "BAGO", "", "TxBAGO" },
    { " ", "", "DAY", "Tx" },
    { " ", "GO", " ", "TOGO" },
    { "", "GETH", "ER", "TxGed" },
    { "", "LER", "A", "TQLx|" },
    { "", "MATO", "", "TxMhTO" },
    { "", "M", "BOY", "TQM" },
    { "", "MB", ":", "TUM" },
    { " ", "MORR", "OW", "TxMQR" },
    { "", "", "NIGHT", "Tx" },
    { "", "NGUE", "", "Tun" },
    { " ", "O", " ", "~TU" },
    { " ", "PHELP", " ", "TQpHe1p" },
    { "", "RNADO", "", "ToN}ADO" },
    { " ", "SHIBA", " ", "TQs}EBx" },
    { "", "T", "AL", "TOT" },
    { "", "UCH", "", "Tuc" },
    { "", "WAR", "D", "TxWo" },
    { "", "W", "ER", "Tw" },
    { "", "WN", "", "TwN" },
};
static const fw_rule_list l_to = { r_to, 22 };

/* lts_rules_t at LOW $6E1D: TR */
static const fw_rule r_tr[] = {
    { "", "ADE", "", "T3AD" },
    { "", "AD", "ITION", "TRxD" },
    { "", "AG", "EDY", "T3aj" },
    { "", "AN", "S", "T3hN" },
    { "", "EACH", "", "T3ec" },
    { " ", "E", "MEND", "T3x" },
    { "", "IB", "AL", "T3I" },
    { " ", "IN", "IDAD", "T3iN" },
    { "", "ITE", "", "T3IT" },
    { "", "IVI", "", "T3iVi" },
    { " ", "IO", "", "T3EO" },
    { " ", "I", "#", "T3I" },
    { "#:!#:!", "", "IES ", "T3" },
    { "#:!#:!", "", "Y ", "T3" },
    { "", "IES", " ", "T3IZ" },
    { "", "O", "PHY", "T3Q" },
    { "", "OUGH", "", "T3QF" },
    { "", "UC", "UL", "T3uK" },
    { "", "Y", " ", "T3I" },
};
static const fw_rule_list l_tr = { r_tr, 19 };

/* lts_rules_t at LOW $6EDF: TU */
static const fw_rule r_tu[] = {
    { "", "", "A", "cU" },
    { "", "", "DE", "TYU" },
    { "", "ES", "DAY", "cUZ" },
    { "", "I", "TION", "TYUi" },
    { " ", "NIS", " ", "TYUNiS" },
    { " ", "NISIA", "", "TYvN}iZJ" },
    { "", "R", "ESQ", "cx" },
    { "", "RE", "", "cx|" },
    { "", "R", "#", "cr|" },
};
static const fw_rule_list l_tu = { r_tu, 9 };

/* lts_rules_t at LOW $6F34: TW */
static const fw_rule r_tw[] = {
    { "", "ICE", "", "TWIS" },
    { " ", "O", "", "TU" },
};
static const fw_rule_list l_tw = { r_tw, 2 };

/* lts_rules_t at LOW $6F44: TY */
static const fw_rule r_ty[] = {
    { "", "", "PHO", "TI" },
};
static const fw_rule_list l_ty = { r_ty, 1 };

static const fw_rule_list *const k_lts_rules_t[] = {
    &l_ta, &l_tc, &l_te, &l_th, &l_ti, &l_to,
    &l_tr, &l_tu, &l_tw, &l_ty,
};
const fw_rule_table lts_rules_t = { "lts_rules_t", 0x6AA2, 1194, true, k_lts_rules_t, 10 };

const fw_rule_list *const lts_pairs_t[26] = {
    ['A' - 'A'] = &l_ta,
    ['C' - 'A'] = &l_tc,
    ['E' - 'A'] = &l_te,
    ['H' - 'A'] = &l_th,
    ['I' - 'A'] = &l_ti,
    ['O' - 'A'] = &l_to,
    ['R' - 'A'] = &l_tr,
    ['U' - 'A'] = &l_tu,
    ['W' - 'A'] = &l_tw,
    ['Y' - 'A'] = &l_ty,
};

/* lts_rules_u at LOW $6F4C: UA */
static const fw_rule r_ua[] = {
    { "#:", "", "L", "YUx" },
};
static const fw_rule_list l_ua = { r_ua, 1 };

/* lts_rules_u at LOW $6F55: UC */
static const fw_rule r_uc[] = {
    { "", "", "+", "YUS" },
};
static const fw_rule_list l_uc = { r_uc, 1 };

/* lts_rules_u at LOW $6F5C: UI */
static const fw_rule r_ui[] = {
    { "#:!", "NE", "", "YUiN" },
    { "D", "", "T", "Ui" },
    { ".", "", "T", "i" },
    { "C", "", "T", "i" },
    { "@", "NG", "", "Uin" },
    { "", "NG", "", "YUin" },
    { "@", "", "", "U" },
    { "", "", "", "YU" },
};
static const fw_rule_list l_ui = { r_ui, 8 };

/* lts_rules_u at LOW $6F90: UK */
static const fw_rule r_uk[] = {
    { " ", "", " ", "YUKA" },
    { " ", "RAIN", "#", "YvKR}AN" },
};
static const fw_rule_list l_uk = { r_uk, 2 };

/* lts_rules_u at LOW $6FA8: UL */
static const fw_rule r_ul[] = {
    { "", "", "AR ", "YvL" },
    { "", "", "AR", "YUL" },
    { "", "", "AT", "YvL" },
    { "", "TIMATE", " ", "u1TiMxT" },
};
static const fw_rule_list l_ul = { r_ul, 4 };

/* lts_rules_u at LOW $6FCF: UM */
static const fw_rule r_um[] = {
    { "@", "", "ENT", "xM" },
    { "", "", "ENT", "YvM" },
    { "", "N", " ", "xM" },
};
static const fw_rule_list l_um = { r_um, 3 };

/* lts_rules_u at LOW $6FE6: UN */
static const fw_rule r_un[] = {
    { " ", "", "ABLE", "uN}" },
    { " ", "ANI", "", "YUNaNi" },
    { " ", "", "ARY", "YUN" },
    { "", "C", "LE", "unK" },
    { " ", "DO", "% ", "uNDU" },
    { " ", "DO", " ", "uNDU" },
    { " ", "DOES", "", "uNDuZ" },
    { " ", "ERRING", "", "uN}rRin" },
    { " ", "", "IDEN", "uN}" },
    { " ", "I", "F", "YUNiF" },
    { " ", "", "IN", "uN}" },
    { " ", "", "IM", "uN}" },
    { "", "I", "TE", "YUNI" },
    { " ", "", "IQ", "YUN" },
    { " ", "I", "", "YUNi" },
    { " ", "TI", "L", "~uNTi" },
    { " ", "LESS", "", "~uNLeS" },
    { " ", "", "", "uN" },
};
static const fw_rule_list l_un = { r_un, 18 };

/* lts_rules_u at LOW $7099: UP */
static const fw_rule r_up[] = {
    { "", "ON", " ", "xPQN" },
    { " ", "", "", "uP" },
};
static const fw_rule_list l_up = { r_up, 2 };

/* lts_rules_u at LOW $70A8: UR */
static const fw_rule r_ur[] = {
    { "#@", "AC", "+", "xRxS" },
    { "#:", "AC", "+", "YxRxS" },
    { "#@", "A", "TE", "xRx" },
    { "#:", "A", "TE", "YxRx" },
    { "", "B", "AN", "rB" },
    { "L", "", "#", "Yx|" },
    { "D", "", "#", "Yx|" },
    { " ", "UGUAY", "", "YoRxGWI" },
    { " :", "", "#", "Yv|" },
    { "", "", "#", "x|" },
    { "#@", "", " ", "x|" },
    { "#:", "", " ", "Yx|" },
    { "", "R", "#", "uR" },
    { "", "", "", "r|" },
};
static const fw_rule_list l_ur = { r_ur, 14 };

/* lts_rules_u at LOW $711F: US */
static const fw_rule r_us[] = {
    { " ", "A", " ", "YUeSA" },
    { "", "", "AGE", "YUS" },
    { " WILL ", "E", "", "YUZ" },
    { " TO ", "E", "", "YUZ" },
    { " I ", "E", "", "YUZ" },
    { " YOU ", "E", "", "YUZ" },
    { " HE ", "", "E", "YUZ" },
    { " SHE ", "", "E", "YUZ" },
    { " IT ", "", "E", "YUZ" },
    { " WE ", "E", "", "YUZ" },
    { " THEY ", "E", "", "YUZ" },
    { "UN", "", "%", "YUZ" },
    { "", "E", " ", "YUS" },
    { " ", "", "%", "YUZ" },
    { "", "E", "", "YUS" },
    { "", "", "UAL", "YUz" },
};
static const fw_rule_list l_us = { r_us, 16 };

/* lts_rules_u at LOW $71B5: UT */
static const fw_rule r_ut[] = {
    { "!", "H", "", "Ut" },
};
static const fw_rule_list l_ut = { r_ut, 1 };

/* lts_rules_u at LOW $71BC: UY */
static const fw_rule r_uy[] = {
    { "", "", "", "I" },
};
static const fw_rule_list l_uy = { r_uy, 1 };

static const fw_rule_list *const k_lts_rules_u[] = {
    &l_ua, &l_uc, &l_ui, &l_uk, &l_ul, &l_um,
    &l_un, &l_up, &l_ur, &l_us, &l_ut, &l_uy,
};
const fw_rule_table lts_rules_u = { "lts_rules_u", 0x6F4C, 628, true, k_lts_rules_u, 12 };

const fw_rule_list *const lts_pairs_u[26] = {
    ['A' - 'A'] = &l_ua,
    ['C' - 'A'] = &l_uc,
    ['I' - 'A'] = &l_ui,
    ['K' - 'A'] = &l_uk,
    ['L' - 'A'] = &l_ul,
    ['M' - 'A'] = &l_um,
    ['N' - 'A'] = &l_un,
    ['P' - 'A'] = &l_up,
    ['R' - 'A'] = &l_ur,
    ['S' - 'A'] = &l_us,
    ['T' - 'A'] = &l_ut,
    ['Y' - 'A'] = &l_uy,
};

/* lts_rules_v at LOW $71C0: VA */
static const fw_rule r_va[] = {
    { "", "C", "AN", "VAk" },
    { "", "GINA", "", "VxjINx" },
    { "", "LET", "", "VaLA" },
    { "", "LID", "ITY", "VxLiD" },
    { "", "LID", "", "VaLiD" },
    { "", "LL", "EY", "VaL" },
    { "", "LU", "#", "VaLYU" },
    { "", "", "PO", "VA" },
    { " ", "RIE", "T", "VxRIx" },
    { "", "RI", "", "VlRi" },
    { " ", "R", "Y", "VlR" },
    { " ", "SE", "", "VhZ" },
    { "#:", "S", "IVE", "VAS" },
};
static const fw_rule_list l_va = { r_va, 13 };

/* lts_rules_v at LOW $723D: VD */
static const fw_rule r_vd[] = {
    { " ", "U", " ", "VEDEYU" },
};
static const fw_rule_list l_vd = { r_vd, 1 };

/* lts_rules_v at LOW $7249: VE */
static const fw_rule r_ve[] = {
    { "", "GET", "A", "VejxT" },
    { "", "HIC", "L", "VEi6" },
    { "", "LO", "CI", "VxLQS" },
    { "", "NER", "A", "VeNxR" },
    { "", "NEZUEL", "A", "VeNxZWAL" },
    { "", "NGEAN", "CE", "VeNjxN" },
    { "#:", "N", "TION", "VeN" },
    { " ", "NUS", "", "VENxS" },
    { " :#:#:", "N", "+", "VEN" },
    { "", "R", "ANCE", "VJ|" },
    { "", "RE", "", "VJ|" },
    { "", "R", "IFY", "VeR" },
    { " ", "R", "Y", "VeR" },
    { " ", "R", " ", "Vr|" },
    { " ", "TO", "", "VETO" },
    { "", "TER", "A", "VeTxR" },
    { ":#:!", "Y", "", "VA" },
    { "'", "", "", "V" },
};
static const fw_rule_list l_ve = { r_ve, 18 };

/* lts_rules_v at LOW $7304: VI */
static const fw_rule r_vi[] = {
    { " ", "A", "", "VIx" },
    { " ", "", "B", "VI" },
    { "", "CE", "", "VIS" },
    { "", "DEO", "", "ViDbO" },
    { "", "E", "T", "VJ" },
    { "", "EW", "", "VYU" },
    { "", "GOR", "", "ViGx|" },
    { "", "GOUR", "", "ViGx|" },
    { "", "LLAI", "N", "ViLx" },
    { "", "NEG", "AR", "ViNiG" },
    { "", "NE", "YA", "ViN" },
    { "", "ND", ":#", "ViND" },
    { " ", "OLA", ": ", "VIOLx" },
    { " ", "O", "", "VIx" },
    { "", "R", "IL+", "ViR" },
    { "", "R", "AL", "VIR" },
    { "", "RUS", "", "VIRuS" },
    { "", "RTU", "", "VrcU" },
    { "", "", "SION", "Vi" },
    { "", "S", "% ", "VIZ" },
    { "", "SIT", "", "ViZiT" },
    { "", "", "S", "Vi" },
    { "", "S", "UAL", "Viz" },
    { "", "", "TAL", "VI" },
    { "", "V", "% ", "VIV" },
    { "", "VI", "D", "ViVi" },
};
static const fw_rule_list l_vi = { r_vi, 26 };

/* lts_rules_v at LOW $73E7: VO */
static const fw_rule r_vo[] = {
    { "", "C", "A!", "VOK" },
    { "", "LCANO", "", "VQLK}ANO" },
    { "", "LCAN", "", "VuLKxN" },
    { "", "LUM", "#", "VQLYUM" },
    { "", "LV", "+", "VQ1V" },
    { "", "", "T+", "VO" },
};
static const fw_rule_list l_vo = { r_vo, 6 };

static const fw_rule_list *const k_lts_rules_v[] = {
    &l_va, &l_vd, &l_ve, &l_vi, &l_vo,
};
const fw_rule_table lts_rules_v = { "lts_rules_v", 0x71C0, 614, true, k_lts_rules_v, 5 };

const fw_rule_list *const lts_pairs_v[26] = {
    ['A' - 'A'] = &l_va,
    ['D' - 'A'] = &l_vd,
    ['E' - 'A'] = &l_ve,
    ['I' - 'A'] = &l_vi,
    ['O' - 'A'] = &l_vo,
};

/* lts_rules_w at LOW $7426: WA */
static const fw_rule r_wa[] = {
    { " ", "GO", "N", "WaGx" },
    { "", "LLE", "T", "WQLi" },
    { "", "", "LLO", "WQ" },
    { "", "", "LL", "Wo" },
    { "", "RE", "HAM", "WlR" },
    { "", "R", "+", "Wl|" },
    { "", "R", "R", "WQ|" },
    { "", "R", "#", "WQ|" },
    { "", "R", "", "Wo|" },
    { "", "", "ST", "WA" },
    { " ", "S", " ", "~WQZ" },
    { "", "", "TER", "Wo" },
    { "", "", "G: ", "Wa" },
    { "", "", "!G", "Wa" },
    { "", "L", "K", "Wo" },
    { "", "", "!K", "Wa" },
    { "", "VE", "", "WAV" },
    { "", "", "!: ", "WQ" },
    { "", "", "!!", "WQ" },
};
static const fw_rule_list l_wa = { r_wa, 19 };

/* lts_rules_w at LOW $74B0: WE */
static const fw_rule r_we[] = {
    { " ", "", "'", "~WE" },
    { " ", "A", "P#", "We" },
    { " ", "AR", "Y", "WJR" },
    { " ", "AR", "INESS", "WJR" },
    { " ", "AR", "", "Wl|" },
    { " ", "ALTH", "", "We1t" },
    { " ", "A", "TH", "We" },
    { "", "DNES", "DAY", "WeNZ" },
    { "", "G", "IAN", "WEj" },
    { " ", "RE", "", "~Wr|" },
    { "", "IRD", "", "WJD" },
};
static const fw_rule_list l_we = { r_we, 11 };

/* lts_rules_w at LOW $7518: WH */
static const fw_rule r_wh[] = {
    { " ", "A", "T", "`WQ" },
    { "", "A", "T", "WQ" },
    { " ", "E", "N", "`We" },
    { " ", "ERE", " ", "`Wl|" },
    { " ", "ER", "EVER", "WlR" },
    { " ", "ERE", "", "~Wl|" },
    { "", "ERE", " ", "Wl|" },
    { " ", "ETH", "ER", "~Wed" },
    { " ", "ICH", "", "`Wic" },
    { " ", "I", "LE", "~WI" },
    { " ", "I", "LST", "~WI" },
    { "", "ITE", "", "WIT" },
    { " ", "OSE", "", "`HUZ" },
    { " ", "O", "M", "`HU" },
    { "", "OLE", "", "HO1" },
    { "", "O", "L", "HO" },
    { "", "O", " ", "`HU" },
    { "", "O", "EVER", "`HU" },
    { "", "O", "SOEVER", "`HU" },
    { " ", "Y", "", "`WI" },
    { "", "", "", "W" },
};
static const fw_rule_list l_wh = { r_wh, 21 };

/* lts_rules_w at LOW $75D1: WI */
static const fw_rule r_wi[] = {
    { " ", "", "LL ", "Wi" },
    { " ", "TH", "", "~Wid" },
    { " ", "DE", "", "WID" },
    { "", "CKE", "D", "WiCi" },
    { "", "DOW", "", "WiDO" },
    { "", "LD", "ERNESS", "Wi1D" },
    { "", "NDOW", "", "WiNDO" },
    { ":", "ND", "!:#:", "WiN" },
    { ":#:!", "ND", ":", "WiN" },
    { "", "RE", "", "WIx|" },
    { "", "SE", "", "WIZ" },
    { "", "ZARD", "", "WiZxD" },
};
static const fw_rule_list l_wi = { r_wi, 12 };

/* lts_rules_w at LOW $7649: WO */
static const fw_rule r_wo[] = {
    { "", "E", "", "WO" },
    { "", "", "KING", "WO" },
    { "", "", "LF", "Wv" },
    { "", "", "LV", "Wv" },
    { "", "MAN", "", "WvMxN" },
    { "", "", "MB", "WU" },
    { "", "MEN", "", "WiMiN" },
    { " ", "N'", "T ", "WON" },
    { " ", "N", " ", "WuN" },
    { " ", "O", "D", "Wv" },
    { "", "R", "CESTER", "Wv" },
    { "", "R", "N", "Wo" },
    { "", "", "RRY", "Wu" },
    { "", "R", "!", "Wr" },
    { "", "VE", "", "WOV" },
    { "", "UL", "D", "Wv" },
    { "", "U", "ND", "WU" },
};
static const fw_rule_list l_wo = { r_wo, 17 };

/* lts_rules_w at LOW $76C9: WR */
static const fw_rule r_wr[] = {
    { "", "ATH", "", "RQt" },
    { "", "ETCHE", "D", "Reci" },
    { "", "I", "T#", "RI" },
    { "", "", "", "R" },
};
static const fw_rule_list l_wr = { r_wr, 4 };

static const fw_rule_list *const k_lts_rules_w[] = {
    &l_wa, &l_we, &l_wh, &l_wi, &l_wo, &l_wr,
};
const fw_rule_table lts_rules_w = { "lts_rules_w", 0x7426, 706, true, k_lts_rules_w, 6 };

const fw_rule_list *const lts_pairs_w[26] = {
    ['A' - 'A'] = &l_wa,
    ['E' - 'A'] = &l_we,
    ['H' - 'A'] = &l_wh,
    ['I' - 'A'] = &l_wi,
    ['O' - 'A'] = &l_wo,
    ['R' - 'A'] = &l_wr,
};

/* lts_rules_x at LOW $76E8: XC */
static const fw_rule r_xc[] = {
    { " ", "OP", "", "ekSKQP" },
};
static const fw_rule_list l_xc = { r_xc, 1 };

/* lts_rules_x at LOW $76F4: XE */
static const fw_rule r_xe[] = {
    { " ", "N", "O", "ZeN" },
};
static const fw_rule_list l_xe = { r_xe, 1 };

/* lts_rules_x at LOW $76FD: XI */
static const fw_rule r_xi[] = {
    { "", "E", "", "ZIe" },
    { "", "", "OUS", "ks" },
    { "", "RE", "", "ksx|" },
    { "", "", "ON", "ks" },
};
static const fw_rule_list l_xi = { r_xi, 4 };

/* lts_rules_x at LOW $7719: XM */
static const fw_rule r_xm[] = {
    { " ", "AS", "", "KRiSMxS" },
    { " ", "IN", " ", "ekSMiN" },
};
static const fw_rule_list l_xm = { r_xm, 2 };

static const fw_rule_list *const k_lts_rules_x[] = {
    &l_xc, &l_xe, &l_xi, &l_xm,
};
const fw_rule_table lts_rules_x = { "lts_rules_x", 0x76E8, 74, true, k_lts_rules_x, 4 };

const fw_rule_list *const lts_pairs_x[26] = {
    ['C' - 'A'] = &l_xc,
    ['E' - 'A'] = &l_xe,
    ['I' - 'A'] = &l_xi,
    ['M' - 'A'] = &l_xm,
};

/* lts_rules_y at LOW $7732: YA */
static const fw_rule r_ya[] = {
    { "", "CH", "T", "YQ" },
    { "", "R", "D: ", "Yh" },
};
static const fw_rule_list l_ya = { r_ya, 2 };

/* lts_rules_y at LOW $7742: YE */
static const fw_rule r_ye[] = {
    { " ", "AR", "", "Yr|" },
    { " ", "MEN", "", "YeMxN" },
    { " ", "S", "", "YeS" },
    { " ", "", "T ", "Ye" },
};
static const fw_rule_list l_ye = { r_ye, 4 };

/* lts_rules_y at LOW $7764: YI */
static const fw_rule r_yi[] = {
    { "#", "NG", "", "Iin" },
    { " :", "NG", "", "Iin" },
    { "PL", "NG", "", "Iin" },
    { "", "NG", "", "Ein" },
};
static const fw_rule_list l_yi = { r_yi, 4 };

/* lts_rules_y at LOW $7786: YO */
static const fw_rule r_yo[] = {
    { "", "GURT", "", "YQGxT" },
    { " ", "UTH", "", "YUt" },
    { "", "UNG", "E", "YunG" },
    { "", "UNG", "", "Yun" },
    { " ", "UR", "", "~Yo|" },
    { "", "U'RE", "", "~Yo|" },
    { " ", "U", "'", "~YU" },
    { " ", "U", " ", "~YU" },
};
static const fw_rule_list l_yo = { r_yo, 8 };

/* lts_rules_y at LOW $77D0: YP */
static const fw_rule r_yp[] = {
    { "!", "E", "", "IP" },
};
static const fw_rule_list l_yp = { r_yp, 1 };

static const fw_rule_list *const k_lts_rules_y[] = {
    &l_ya, &l_ye, &l_yi, &l_yo, &l_yp,
};
const fw_rule_table lts_rules_y = { "lts_rules_y", 0x7732, 165, true, k_lts_rules_y, 5 };

const fw_rule_list *const lts_pairs_y[26] = {
    ['A' - 'A'] = &l_ya,
    ['E' - 'A'] = &l_ye,
    ['I' - 'A'] = &l_yi,
    ['O' - 'A'] = &l_yo,
    ['P' - 'A'] = &l_yp,
};

/* lts_rules_z at LOW $77D7: ZA */
static const fw_rule r_za[] = {
    { " ", "IRE", "", "ZIEx|" },
    { "", "N", "Y", "ZAN" },
};
static const fw_rule_list l_za = { r_za, 2 };

/* lts_rules_z at LOW $77EA: ZE */
static const fw_rule r_ze[] = {
    { "", "A", "LA", "ZE" },
    { "", "RO", "", "ZJRO" },
};
static const fw_rule_list l_ze = { r_ze, 2 };

/* lts_rules_z at LOW $77FA: ZI */
static const fw_rule r_zi[] = {
    { "", "MBABWE", "", "ZiMB}hBWA" },
};
static const fw_rule_list l_zi = { r_zi, 1 };

/* lts_rules_z at LOW $780C: ZO */
static const fw_rule r_zo[] = {
    { "", "O", ": ", "ZU" },
    { "", "", "O", "ZO" },
};
static const fw_rule_list l_zo = { r_zo, 2 };

/* lts_rules_z at LOW $7819: ZU */
static const fw_rule r_zu[] = {
    { "", "LU", "", "ZULU" },
    { "", "RE", "", "ZYq|" },
};
static const fw_rule_list l_zu = { r_zu, 2 };

/* lts_rules_z at LOW $782A: ZY */
static const fw_rule r_zy[] = {
    { "", "ME", "", "ZIM" },
};
static const fw_rule_list l_zy = { r_zy, 1 };

static const fw_rule_list *const k_lts_rules_z[] = {
    &l_za, &l_ze, &l_zi, &l_zo, &l_zu, &l_zy,
};
const fw_rule_table lts_rules_z = { "lts_rules_z", 0x77D7, 91, true, k_lts_rules_z, 6 };

const fw_rule_list *const lts_pairs_z[26] = {
    ['A' - 'A'] = &l_za,
    ['E' - 'A'] = &l_ze,
    ['I' - 'A'] = &l_zi,
    ['O' - 'A'] = &l_zo,
    ['U' - 'A'] = &l_zu,
    ['Y' - 'A'] = &l_zy,
};
