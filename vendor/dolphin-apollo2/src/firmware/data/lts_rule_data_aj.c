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


/* lts_rules_a at $5625: AA */
static const fw_rule r_aa[] = {
    { "", "R", "", "h|" },
    { "", "", "", "h" },
};
static const fw_rule_list l_aa = { r_aa, 2 };

/* lts_rules_a at $562E: AB */
static const fw_rule r_ab[] = {
    { " ", "AC", "US", "aBxK" },
    { " ", "", "ANDON", "xB" },
    { "", "BRE", "", "xBRE" },
    { " ", "EL", "", "xBEL" },
    { " ", "", "ERR", "aB" },
    { " ", "", "EY", "xB" },
    { " ", "", "H", "xB" },
    { "", "", "ILIT", "xB" },
    { "", "JURE", "", "aBjq|" },
    { " :", "", "L+", "AB" },
    { "US", "", "L+", "xB" },
    { " #:", "", "L+", "AB" },
    { "", "", "LE: ", "xB" },
    { "", "", "LY ", "xB" },
    { "", "OR", "IG", "aBxR" },
    { " ", "", "O", "xB" },
    { "", "RAS", "IV", "xBRAS" },
    { " ", "S", "EN", "aBS" },
    { " ", "SO", "LU", "aBSQ" },
    { " ", "", "SOR", "xB" },
    { " ", "", "SU", "xB" },
    { " ", "", "U", "xB" },
};
static const fw_rule_list l_ab = { r_ab, 22 };

/* lts_rules_a at $56EB: AC */
static const fw_rule r_ac[] = {
    { "", "ADEM", "IC", "aKxDeM" },
    { "", "ADEM", "", "xKaDxM" },
    { "", "CE", "!:", "aKSe" },
    { " ", "CO", "LADE", "aKx" },
    { " ", "COUT", "RE", "xKUT" },
    { " ", "", "CO", "x" },
    { " ", "C", "R", "x6" },
    { "", "CURSE", "D", "aKrSi" },
    { " ", "HA", "EA: ", "xK" },
    { ":!#:", "H", "% ", "AK" },
    { " ", "H", "% ", "AK" },
    { " ", "HILLES", "", "xCiLEZ" },
    { " ", "H", "IEV", "xc" },
    { " ", "K", " ", "ak" },
    { " ", "", "KN", "xK" },
    { "", "", "ORN", "A6" },
    { "", "OU", "STIC", "x6U" },
    { " ", "", "QUIESC", "a" },
    { " ", "QU", "#:#TION", "a6W" },
    { " ", "", "QU", "x" },
    { "", "RE", "", "AKx|" },
    { "", "R", "OSS", "xKR" },
    { " ", "R", "O", "aKR" },
    { "", "", "ULAR", "a6" },
    { "", "", "Y", "xS" },
    { "", "", "IES", "xS" },
};
static const fw_rule_list l_ac = { r_ac, 26 };

/* lts_rules_a at $57E5: AD */
static const fw_rule r_ad[] = {
    { "", "A", "PT", "xDa" },
    { "#:!", "A", " ", "hDx" },
    { "", "EQUA", "T", "aDi6Wx" },
    { ":!", "", "% ", "AD" },
    { "", "HER", "E", "xDHJ|" },
    { "", "HE", "S+", "xDHE" },
    { "", "MIR", "AL", "aDMiR" },
    { "", "O", "PT", "aDQ" },
    { "#:", "OR", "", "aDo|" },
    { " ", "O", " ", "xDU" },
    { "", "O", ": ", "hDO" },
    { "", "O", "%", "hDO" },
    { "", "VAN", "!#", "xDVhN" },
    { "", "", "VICE", "xD" },
    { "", "VIS", "", "xDVIZ" },
    { "", "VERTIS", "%", "aDVxTIZ" },
};
static const fw_rule_list l_ad = { r_ad, 16 };

/* lts_rules_a at $5885: AE */
static const fw_rule r_ae[] = {
    { "", "R", "", "l|" },
    { " ", "", "", "E" },
    { "", "", " ", "E" },
};
static const fw_rule_list l_ae = { r_ae, 3 };

/* lts_rules_a at $5893: AF */
static const fw_rule r_af[] = {
    { " ", "AR", " ", "xFh|" },
    { " ", "F", "EC", "xF" },
    { "", "F", "L+", "aF" },
    { "", "FI", "C", "aFi" },
    { "", "R", "ICA", "aFR" },
    { " ", "", "T", "hF" },
    { " ", "F", "#", "xF" },
    { " ", "", ":#", "aF" },
    { "", "", "!", "hF" },
};
static const fw_rule_list l_af = { r_af, 9 };

/* lts_rules_a at $58D7: AG */
static const fw_rule r_ag[] = {
    { "", "AIN", "", "xGeN" },
    { "", "ENDA", "", "xjeNDx" },
    { "", "EN", "", "AjxN" },
    { "#:!", "E", "OUS", "Aj" },
    { "#:", "", "% ", "ij" },
    { "#:", "E", "", "ij" },
    { " ", "", "E", "Aj" },
    { "", "ONIS", "#", "aGxNIZ" },
    { " ", "ORA", "", "aGRx" },
    { "", "R", "EE", "xGR" },
    { " ", "RI", "", "aGRi" },
};
static const fw_rule_list l_ag = { r_ag, 11 };

/* lts_rules_a at $593D: AH */
static const fw_rule r_ah[] = {
    { "", "", " ", "h" },
    { "", "", "", "xH" },
};
static const fw_rule_list l_ah = { r_ah, 2 };

/* lts_rules_a at $5946: AI */
static const fw_rule r_ai[] = {
    { "", "", "C", "Ai" },
    { "", "GN", "", "AN" },
    { ":#:", "N", "LY ", "xN" },
    { " A:#:", "N", "", "AN" },
    { ":#:#:", "N", ": ", "xN" },
    { "", "RE", ": ", "l|" },
    { "", "R", "", "l|" },
    { "", "S", "LE", "I" },
    { "", "", "", "A" },
};
static const fw_rule_list l_ai = { r_ai, 9 };

/* lts_rules_a at $5989: AK */
static const fw_rule r_ak[] = {
    { "", "", "E", "AK" },
};
static const fw_rule_list l_ak = { r_ak, 1 };

/* lts_rules_a at $598F: AL */
static const fw_rule r_al[] = {
    { "", "", "ARM", "xL" },
    { "", "", "BANIA", "aL" },
    { " ", "BEIT", " ", "o1BEiT" },
    { " ", "B", "INO", "a1B" },
    { " ", "B", "ION", "a1B" },
    { "", "COHO", "L", "aLKxHQ" },
    { "", "CHEM", "", "aLKxM" },
    { " ", "", "ER", "xL" },
    { "", "GER", "IA", "aLjER" },
    { "", "GOR", "I", "aLGxR" },
    { "", "", "IA", "AL" },
    { "", "", "IGN", "xL" },
    { "", "", "IN ", "xL" },
    { "#:", "", "ION", "aL" },
    { "#:", "", "IT", "aL" },
    { "", "IVE", "", "xLIV" },
    { "#:", "", "ISE", "xL" },
    { "#:", "", "IZE", "xL" },
    { " ", "KA", "!", "a1Kx" },
    { " ", "", "K", "a1" },
    { "", "", "K", "o" },
    { "", "L", "ATION", "xL" },
    { "", "LE", "GAT", "aLx" },
    { "", "LE", "GE", "xLe" },
    { "", "LERG", "IC", "aLrj" },
    { "", "LERG", "Y", "aLrj" },
    { " ", "LE", "!I", "xLE" },
    { "", "LIE", "!", "aLI" },
    { "", "LO", "CAT+", "aLx" },
    { " ", "", "ONE", "xL" },
    { "", "LO", "TROPE", "aLx" },
    { " ", "L", "OT", "xL" },
    { " !", "LOW", "", "aLO" },
    { "", "LOW", "", "xLw" },
    { "", "", "LURE", "x" },
    { "", "L", "% ", "oL" },
    { "", "L", "S ", "oL" },
    { "", "L", " ", "oL" },
    { " ", "LY", "", "aLI" },
    { ":#:", "", "LY", "x" },
    { "", "MO", "ST", "o1MO" },
    { "", "M", "", "hM" },
    { "", "PHA", "", "a1Fx" },
    { "", "", "R", "oL" },
    { "#:", "", "S ", "x1" },
    { "", "", "S", "o1" },
    { "", "TERNA", "T+", "o1TrNx" },
    { "#:", "", "T ", "x1" },
    { "", "", "TIT", "a1" },
    { "", "", "T", "o1" },
    { "", "UMIN", "IUM", "aLYvMiN" },
    { "!", "V", "+", "hV" },
    { "", "", "W", "oL" },
    { "", "", "!", "a1" },
    { " :#:#:!", "", "Y ", "xL" },
};
static const fw_rule_list l_al = { r_al, 55 };

/* lts_rules_a at $5B72: AM */
static const fw_rule r_am[] = {
    { " ", "", " ", "~aM" },
    { "", "ATEUR", "", "aMxTx|" },
    { "", "BASS", "A", "aMBaS" },
    { "CH", "B", "ER", "AMB" },
    { " ", "B", "ER", "aMB" },
    { "", "B", "IGU", "aMB" },
    { " ", "EN", " ", "hMeN" },
    { " ", "END", "", "xMeND" },
    { " ", "", "ER", "xM" },
    { " ", "", "ID", "xM" },
    { "", "ONG", "", "xMun" },
    { "", "OUR", "", "aMx|" },
    { " ", "PHI", "", "aMFi" },
};
static const fw_rule_list l_am = { r_am, 13 };

/* lts_rules_a at $5BEE: AN */
static const fw_rule r_an[] = {
    { " ", "", " ", "~aN" },
    { " ", "", "AL ", "AN" },
    { "", "A", "LOGU", "aNx" },
    { "", "A", "LOG#", "xNa" },
    { "", "A", "LYSI", "xNa" },
    { "", "A", "L", "aNx" },
    { "", "", "ARCH", "aN" },
    { "", "ATOM", "IC", "aNxTQM" },
    { "", "ATOM", "", "xNaTxM" },
    { " :!", "", "C%", "hN" },
    { " !#:", "", "CE: ", "aN" },
    { ":#:", "", "CE: ", "xN" },
    { ":#:", "", "CIAL", "aN" },
    { "", "CHOR", "", "anKx|" },
    { "", "", "CIE", "AN" },
    { ":#:", "", "CY ", "xN" },
    { " ", "", "D ", "~aN" },
    { "", "", "D", "aN" },
    { "#:#:", "E", "OUS", "ANb" },
    { "#:#:", "", "ENT", "xN" },
    { "", "GE", "L", "ANjx" },
    { " ", "G", "ER", "anG" },
    { "", "GE", ": ", "aNj" },
    { "", "G", "%", "an" },
    { "", "G", "S ", "an" },
    { "", "G", "I", "aNj" },
    { "", "G", "L", "anG" },
    { "", "GOL", "A", "anGOL" },
    { "", "G", "R", "anG" },
    { "", "", "G#", "an" },
    { "", "G", "", "an" },
    { "", "", "IA", "AN" },
    { "", "", "IED", "aN" },
    { "", "", "ISM", "xN" },
    { "", "", "K", "an" },
    { "", "NIHILA", "T", "xNIxLA" },
    { "", "N", "OUN", "xN" },
    { "", "N", "", "aN" },
    { "", "O", "D#", "aNO" },
    { "", "", "OTHER", "xN" },
    { "", "", "SION", "aN" },
    { " ", "S", "I ", "aNS" },
    { "", "SW", "", "hNS" },
    { "", "'", "T", "hN" },
    { "", "", "TENNA", "aN" },
    { " ", "TE", "", "aNTb" },
    { "", "", "TIAL", "aN" },
    { " ", "TI", "", "aNTb" },
    { " ", "US", " ", "ANxS" },
    { " :", "", "Y", "eN" },
    { "", "", "Y", "xN" },
    { " ", "", ": ", "aN" },
    { "#:", "", "T: ", "xN" },
    { "#:", "T", "LY ", "xNT" },
    { " ", "", "X", "an" },
    { " ", "", "", "aN" },
    { "", "", "!:", "aN" },
    { "", "", "% ", "AN" },
    { ":#:", "", ": ", "xN" },
};
static const fw_rule_list l_an = { r_an, 59 };

/* lts_rules_a at $5DDC: AP */
static const fw_rule r_ap[] = {
    { "", "ARTHEID", "", "xPhTHAT" },
    { "", "ER", "TURE", "aPx" },
    { "", "EX", "", "APekS" },
    { " ", "OL", "I", "APxL" },
    { " ", "O", "", "xPQ" },
    { "", "PA", "L", "xPo" },
    { "", "PAR", "ENT", "xPaR" },
    { "", "PL", "ICAT", "aPL" },
    { "", "P", "LE", "aP" },
    { " ", "", "PLY", "x" },
    { " ", "", "PL+", "x" },
    { "", "PREH", "#", "aPRiH" },
    { "", "PROV", "#", "xPRUV" },
    { "", "PR", "OX", "xPR" },
    { "", "PRO", "!", "xPRO" },
    { " ", "", "PR", "x" },
    { " ", "P", "#:", "xP" },
    { " ", "R", "", "APR" },
};
static const fw_rule_list l_ap = { r_ap, 18 };

/* lts_rules_a at $5E86: AR */
static const fw_rule r_ar[] = {
    { " ", "CH", "A", "hK" },
    { " ", "CHIVE", "", "hCIV" },
    { " :", "CH", "% ", "hc" },
    { " ", "CH", "I", "hC" },
    { " ", "CH", "!", "hc" },
    { ":!#:", "CH", "", "hK" },
    { "W", "", "", "o|" },
    { "#:D", "", "D", "x" },
    { " ", "EA", "", "lRJ" },
    { " ", "E", " ", "~h|" },
    { "", "", "EE", "x|" },
    { "", "E", "#", "l|" },
    { " ", "EN'", "T ", "~hN" },
    { " ", "E", "N ", "h" },
    { " ", "EN", "", "aREN" },
    { "", "", "EN", "l|" },
    { "", "ER", "", "lRx|" },
    { "", "ESS", "", "aReS" },
    { "", "E", "ST", "lRe" },
    { "", "ET", " ", "aRA" },
    { "", "E", "", "l|" },
    { " ", "GENTINA", "", "hjxNTENx" },
    { " ", "GENT", "", "hjxNT" },
    { ":#:!", "", "IA:", "lR" },
    { "", "I", "LY", "xRx" },
    { " :", "", "%", "l|" },
    { "", "ISON", ": ", "aRiS7" },
    { "#:", "", "ITY", "aR" },
    { " ", "MEN", "IA", "hMEN" },
    { " ", "", "O", "x|" },
    { "", "RE", "ST", "xRe" },
    { "", "RE", " ", "h|" },
    { "", "RI", "V", "xRI" },
    { "", "ROW", "", "aRO" },
    { "", "", "R", "a|" },
    { "#:D", "", "Y", "3" },
    { "#:T", "", "Y", "3" },
    { "#!!", "", "Y", "R" },
    { "", "", "#", "a|" },
    { " :", "", " ", "h|" },
    { "", "", "LY ", "x" },
    { "", "", " ", "x|" },
    { "", "", "", "h|" },
};
static const fw_rule_list l_ar = { r_ar, 43 };

/* lts_rules_a at $5FE3: AS */
static const fw_rule r_as[] = {
    { " ", "", " ", "~aZ" },
    { ":!#:!", "E", " ", "xS" },
    { "", "H", "", "as" },
    { " ", "", "IA", "Az" },
    { " ", "", "IDE", "xS" },
    { " ", "", "K: ", "hS" },
    { " ", "", "K% ", "hS" },
    { "", "PIR", "IN", "aSp3" },
    { "", "SA", "SS", "xSa" },
    { "", "", "SUM", "x" },
    { "", "SUR", "", "xsq|" },
    { "", "S", "ION", "as" },
    { "", "S", "IC", "aS" },
    { "", "CII", "", "aSCb" },
    { " ", "S", "E", "xS" },
    { " ", "S", "O", "xS" },
    { "", "S", "IGNA", "aS" },
    { "", "S", "IGN", "xS" },
    { " ", "YNCH", "", "ASinK" },
    { "#:!", "", "Y ", "xS" },
    { " !", "", "#", "AS" },
    { ":!", "", " ", "xZ" },
};
static const fw_rule_list l_as = { r_as, 22 };

/* lts_rules_a at $6097: AT */
static const fw_rule r_at[] = {
    { " ", "", " ", "~aT" },
    { "!", "A", " ", "ATx" },
    { " ", "E", " ", "AT" },
    { "#:#:", "E", "LY", "xT" },
    { "SSION", "E", "", "xT" },
    { "TION", "E", "", "xT" },
    { "#:", "", "EDLY", "AT" },
    { "#:", "", "ED ", "AT" },
    { "", "H", "ER", "hd" },
    { " :", "", "ION#", "as" },
    { "", "", "ION", "As" },
    { "#:!", "IVE", "", "xTiV" },
    { " ", "LAS", "", "aTLxS" },
    { "", "OR", "", "ATx|" },
    { ":!#:!", "UR", "+", "xcq|" },
    { "", "TORN", "EY", "xTrN" },
    { " ", "T", "IT", "aT" },
    { " ", "TR", "IB", "aT3" },
    { " ", "T", "", "xT" },
    { "!", "UM", " ", "ATvM" },
    { "!", "US", " ", "ATvS" },
};
static const fw_rule_list l_at = { r_at, 21 };

/* lts_rules_a at $615C: AU */
static const fw_rule r_au[] = {
    { " ", "N", "T", "hN" },
    { "", "R", "", "o|" },
    { "", "STRAL", "IA", "QSf3AL" },
    { "", "", "STRIA", "Q" },
    { "", "TOMOBI", "L", "oTxMOBE" },
    { "", "TO", "C", "oTx" },
    { "", "TOEXE", "C", "oTOeGZe" },
    { "", "TON", "", "oTQN" },
    { "", "TH", "OR", "ot" },
    { "", "", "", "o" },
};
static const fw_rule_list l_au = { r_au, 10 };

/* lts_rules_a at $61BB: AV */
static const fw_rule r_av[] = {
    { "", "", "A ", "hV" },
    { "", "", "AIL", "xV" },
    { "", "ENUE", "", "aVxNYU" },
    { " ", "ER", "AG%", "aVx|" },
    { " ", "ER", "", "xVr|" },
};
static const fw_rule_list l_av = { r_av, 5 };

/* lts_rules_a at $61EA: AW */
static const fw_rule r_aw[] = {
    { "", "ARE", "", "xWl|" },
    { "", "", "A", "xW" },
    { " ", "", "EIGH", "xW" },
    { " ", "E", "", "o" },
    { "", "", "O", "xW" },
    { "", "H", "ILE", "xW" },
    { "", "", "", "o" },
};
static const fw_rule_list l_aw = { r_aw, 7 };

/* lts_rules_a at $6217: AY */
static const fw_rule r_ay[] = {
    { "", "", "", "A" },
};
static const fw_rule_list l_ay = { r_ay, 1 };

static const fw_rule_list *const k_lts_rules_a[] = {
    &l_aa, &l_ab, &l_ac, &l_ad, &l_ae, &l_af,
    &l_ag, &l_ah, &l_ai, &l_ak, &l_al, &l_am,
    &l_an, &l_ap, &l_ar, &l_as, &l_at, &l_au,
    &l_av, &l_aw, &l_ay,
};
const fw_rule_table lts_rules_a = { "lts_rules_a", 0x5625, 3062, false, k_lts_rules_a, 21 };

const fw_rule_list *const lts_pairs_a[26] = {
    ['A' - 'A'] = &l_aa,
    ['B' - 'A'] = &l_ab,
    ['C' - 'A'] = &l_ac,
    ['D' - 'A'] = &l_ad,
    ['E' - 'A'] = &l_ae,
    ['F' - 'A'] = &l_af,
    ['G' - 'A'] = &l_ag,
    ['H' - 'A'] = &l_ah,
    ['I' - 'A'] = &l_ai,
    ['K' - 'A'] = &l_ak,
    ['L' - 'A'] = &l_al,
    ['M' - 'A'] = &l_am,
    ['N' - 'A'] = &l_an,
    ['P' - 'A'] = &l_ap,
    ['R' - 'A'] = &l_ar,
    ['S' - 'A'] = &l_as,
    ['T' - 'A'] = &l_at,
    ['U' - 'A'] = &l_au,
    ['V' - 'A'] = &l_av,
    ['W' - 'A'] = &l_aw,
    ['Y' - 'A'] = &l_ay,
};
/* lts_rules_b at $621B: BA */
static const fw_rule r_ba[] = {
    { "", "", "BOON", "Bx" },
    { "", "BYLO", "NI", "BaBbLO" },
    { "", "BYLO", "N", "BaBbLQ" },
    { "", "", "BY", "BA" },
    { "", "", "GUET", "Ba" },
    { " ", "HAMA", "S", "BxHhMx" },
    { "", "KELI", "T% ", "BAKxLI" },
    { "", "LA", "", "BaLx" },
    { "", "LCO", "NY", "BaLKx" },
    { "", "LLET", "", "BaLA" },
    { "", "", "LLOON", "Bx" },
    { "", "", "LONEY", "Bx" },
    { "", "N", "AL+", "BxN" },
    { "", "NA", "L", "BxNh" },
    { "", "NQUE", "T", "Ban6Wi" },
    { "", "NGLA", "", "BaNGLx" },
    { "", "RGE", "", "Bhj" },
    { "", "RI", "TONE", "BaRi" },
    { "", "RRAG", "+", "BaRhz" },
    { "", "RRA", "SS", "BaRx" },
    { " ", "", "SS ", "BA" },
    { "", "SI", "N", "BASx" },
    { " ", "TH", "Y", "Bat" },
    { "", "SE", "", "BAS" },
    { "", "ZAAR", "", "BxZh|" },
    { "", "ZAR", "", "BxZh|" },
};
static const fw_rule_list l_ba = { r_ba, 26 };

/* lts_rules_b at $6326: BE */
static const fw_rule r_be[] = {
    { " ", "A", "D", "BE" },
    { "", "AR", "D", "BJ" },
    { "", "AR", "", "Bl|" },
    { "", "AU", "T", "BYU" },
    { " ", "CAUSE", "", "{Bi6QZ" },
    { "", "GAN", "", "{BiGaN" },
    { "", "GIN", "", "{BiGiN" },
    { "", "HE", "ST", "{BiHe" },
    { "", "", "ING", "BE" },
    { " ", "LIZE", "", "BxLEZ" },
    { " ", "NEFI", "T", "BeNiFi" },
    { " ", "NIN", " ", "BeNEN" },
    { "", "RET", "", "BeRA" },
    { " ", "R", "KS", "Bh" },
    { "", "RR", "+", "BeR" },
    { " ", "R", "!", "Br|" },
    { "", "S", "EECH", "{BiS" },
    { " ", "", "STIAL", "Be" },
    { " ", "", "S!:#", "{Bi" },
    { " ", "", "TA", "BE" },
    { " ", "", "TW", "{Bi" },
    { " ", "", "TR", "{Bi" },
    { " ", "", "VER", "Be" },
    { " ", "WI", "LD", "{BiWi" },
    { " ", "YO", "ND", "{BiYQ" },
    { " ", "", "!!", "Be" },
    { " ", "", "!#!:", "{Bi" },
};
static const fw_rule_list l_be = { r_be, 27 };

/* lts_rules_b at $6428: BH */
static const fw_rule r_bh[] = {
    { "", "UTAN", "", "BUThN" },
    { " ", "", "", "B" },
};
static const fw_rule_list l_bh = { r_bh, 2 };

/* lts_rules_b at $6438: BI */
static const fw_rule r_bi[] = {
    { " ", "AFRA", " ", "BbaFRx" },
    { " ", "AS", "", "BIxS" },
    { " ", "", "A", "BI" },
    { "", "", "BLE", "BI" },
    { " ", "", "CK", "Bi" },
    { " ", "", "C", "BI" },
    { " ", "GO", "T", "BiGx" },
    { " ", "", "P:#", "BI" },
    { " ", "", "SON", "BI" },
    { " ", "", "S+", "BI" },
    { "", "NAR", "Y", "BINxR" },
    { "#:", "N", "AT", "BiN" },
    { "#:", "N", "+", "BIN" },
    { "", "NOM", "IAL", "BINOM" },
    { " ", "OS", " ", "BIQS" },
    { " ", "O", " ", "BIO" },
    { "", "", "O!", "BI" },
    { "", "", "TUM", "Bi" },
    { "", "", "T% ", "BI" },
};
static const fw_rule_list l_bi = { r_bi, 19 };

/* lts_rules_b at $64DC: BL */
static const fw_rule r_bl[] = {
    { "", "AME", "", "BLAM" },
    { "", "ANCMANG", "E", "BLxMQNj" },
    { "", "AT", "ANT", "BLAT" },
    { "", "OO", "D", "BLu" },
    { "", "UE", "", "BLU" },
};
static const fw_rule_list l_bl = { r_bl, 5 };

/* lts_rules_b at $6511: BO */
static const fw_rule r_bo[] = {
    { "", "", "D+", "BQ" },
    { " ", "GOTA", "", "BQGxTh" },
    { " ", "", "GUS", "BO" },
    { " ", "HEM", "IA", "BOHEM" },
    { " ", "LIV", "IA", "BxLiV" },
    { "", "", "ND", "BQ" },
    { " ", "NE", "", "BON" },
    { "", "NUS", "", "BONxS" },
    { " ", "N", "!:#", "BQN" },
    { " ", "RE", "", "Bo|" },
    { " ", "ROUGH", "", "BuRx" },
    { "#:", "ROUGH", "", "BxRx" },
    { "", "RO", " ", "BxRx" },
    { " ", "TH", " ", "BOt" },
    { " ", "TSWANA", "", "BQTSWhNx" },
    { "", "UGH", "T", "Bo" },
    { "", "UGH", "", "Bw" },
    { "", "U", "LD", "BO" },
    { "", "UQUET", "", "BUKA" },
    { "", "W", "", "BO" },
};
static const fw_rule_list l_bo = { r_bo, 20 };

/* lts_rules_b at $65D3: BR */
static const fw_rule r_br[] = {
    { "", "ACE", "", "BRAS" },
    { "", "AC", "+", "BRAS" },
    { " ", "ACK", "Y", "BRaC" },
    { "", "AE", "", "BRA" },
    { "", "AVE", "", "BRAV" },
    { " ", "AVO", " ", "BRhVO" },
    { " ", "A", "ZIL", "BRx" },
    { "", "EAKFA", "ST", "BReKFx" },
    { "", "EA", "K", "BRA" },
    { "", "EA", "ST", "BRe" },
    { "", "EA", "TH% ", "BRE" },
    { "", "IDGE", "", "BRij" },
    { "", "IDE", "", "BRID" },
    { "", "IEF", "", "BREF" },
    { " ", "IS", " ", "BRE" },
    { "", "ITAIN", "", "BRiTxN" },
    { " ", "I", "T", "BRi" },
    { "", "OA", "D", "BRo" },
    { "", "OM", "I", "BROM" },
    { "", "ONCH", "", "BRQnK" },
};
static const fw_rule_list l_br = { r_br, 20 };

/* lts_rules_b at $6698: BU */
static const fw_rule r_bu[] = {
    { "", "FFAL", "O", "BuFxL" },
    { "", "", "GL#", "BYU" },
    { "", "I", "L", "Bi" },
    { " ", "", "LL", "Bv" },
    { "", "RG", "ER", "BrG" },
    { "", "RGH", " ", "BuRx" },
    { "", "REAU", "CRAT", "BYqRx" },
    { "", "REAU", "CR", "BYqRQ" },
    { "", "REAU", "", "BYqRO" },
    { "", "", "R+", "Be" },
    { "", "RMAH", "", "BrMx" },
    { "", "S", "ES", "BuS" },
    { "", "S", "MAN", "BuS" },
    { "", "SINESS", "", "BiZNiS" },
    { " ", "S", "+", "BiZ" },
    { "", "", "SH", "Bv" },
    { " ", "T", " ", "~BuT" },
};
static const fw_rule_list l_bu = { r_bu, 17 };

/* lts_rules_b at $673A: BY */
static const fw_rule r_by[] = {
    { " ", "", " ", "~BI" },
    { "", "TE", "", "BIT" },
    { "NEAR", "", " ", "BI" },
    { " :!#RE", "", " ", "BI" },
};
static const fw_rule_list l_by = { r_by, 4 };

static const fw_rule_list *const k_lts_rules_b[] = {
    &l_ba, &l_be, &l_bh, &l_bi, &l_bl, &l_bo,
    &l_br, &l_bu, &l_by,
};
const fw_rule_table lts_rules_b = { "lts_rules_b", 0x621B, 1346, false, k_lts_rules_b, 9 };

const fw_rule_list *const lts_pairs_b[26] = {
    ['A' - 'A'] = &l_ba,
    ['E' - 'A'] = &l_be,
    ['H' - 'A'] = &l_bh,
    ['I' - 'A'] = &l_bi,
    ['L' - 'A'] = &l_bl,
    ['O' - 'A'] = &l_bo,
    ['R' - 'A'] = &l_br,
    ['U' - 'A'] = &l_bu,
    ['Y' - 'A'] = &l_by,
};

/* lts_rules_c at $675D: CA */
static const fw_rule r_ca[] = {
    { " ", "BINE", "T", "6aBiNx" },
    { " ", "", "BL+", "6A" },
    { " ", "CH", "EX", "6aK" },
    { " ", "CHOU", "", "6asU" },
    { " ", "CH", "+", "6as" },
    { "", "", "DENC", "6A" },
    { "", "E", "SAR", "SE" },
    { "", "FE", "", "6aFA" },
    { "", "F\220", "", "6aFA" },
    { "", "FFEIN", "", "6aFEN" },
    { " ", "KE", "", "6Ak" },
    { " ", "LCULA", "T", "6a16YxLA" },
    { "", "", "LEND", "6a" },
    { "", "LE", "DON", "6aLx" },
    { "", "LF", "", "6hF" },
    { "#:#:", "LL", "+", "KxL" },
    { "", "LL", "O", "6aL" },
    { "", "LL", "IGR", "KxL" },
    { " ", "", "LL", "6o" },
    { "", "", "MBRID", "CA" },
    { "", "MBOD", "IA", "CaMBOD" },
    { "", "MERA", "", "CaMRx" },
    { "", "MO", "MILE", "CaMx" },
    { "", "MOUFLAGE", "", "CaMxFLhz" },
    { " ", "ME", "L", "6aMx" },
    { " ", "N", "ADIAN", "KxN" },
    { " ", "NADA", "", "6aNxDx" },
    { " ", "N", " ", "6aN" },
    { " ", "N'", "T", "6hN" },
    { "", "NCEL", "", "KaNS\300" },
    { "", "NC", "ER", "KaNS" },
    { "", "NINE", "", "KANIN" },
    { "", "NOE", "", "KxNU{" },
    { "", "NON", "IC", "KxNQN" },
    { "", "", "NTER", "Ka" },
    { " ", "NYO", "N", "KaNYx" },
    { "", "P", "AB", "6AP" },
    { "", "PITO", "L", "6aPiTx" },
    { " ", "P", "ABLE", "CAP" },
    { " ", "PA", "C", "KxPa" },
    { "", "PRICE", "", "KxpRbS" },
    { "", "PRI", "CORN", "KapRi" },
    { " ", "PTAIN", "", "6aPTiN" },
    { "", "RBINE", "", "6hBIN" },
    { "", "RC", "ASE", "6hk" },
    { " ", "R", "%", "Cl|" },
    { " ", "RR", "+", "6aR" },
    { "", "RLISLE", "", "6hLI1" },
    { "", "SE", "", "KAS" },
    { " ", "SS", "ETTE", "KxS" },
    { " ", "S", "UAL", "KaZ" },
    { "", "SINO", "", "KxSENO" },
    { "", "TARRH", "", "KxTh|" },
    { "", "TE", "GOR", "CaTx" },
    { "", "TER", "HAM", "CATx|" },
    { "", "TER", "PILLAR", "CaTx|" },
    { "", "THED", "", "6atED" },
    { "", "TTERI", "CK", "6aT3i" },
    { " ", "TA", "", "6aTa" },
    { "", "ULI", "", "KQLi" },
    { "", "", "VERN", "Ka" },
};
static const fw_rule_list l_ca = { r_ca, 61 };

/* lts_rules_c at $69EB: CC */
static const fw_rule r_cc[] = {
    { "", "I", "", "kSi" },
};
static const fw_rule_list l_cc = { r_cc, 1 };

/* lts_rules_c at $69F2: CE */
static const fw_rule r_ce[] = {
    { "O", "", "ANIC", "sb" },
    { "O", "AN", "", "sxN" },
    { "", "D", "+", "SED" },
    { "", "IV", "+", "SEV" },
    { " ", "L", "E", "SeL" },
    { " ", "M", "ET", "SeM" },
    { "", "NTRE", "", "SeNTx|" },
    { " ", "REA", "L", "SERJ" },
    { "", "REM", "O", "SeRxM" },
    { "", "RTO", " ", "clTO" },
    { "", "ST", "ER", "Sf" },
    { " ", "RTAIN", "", "SrTxN" },
    { "", "ASE", "", "SES" },
};
static const fw_rule_list l_ce = { r_ce, 13 };

/* lts_rules_c at $6A6D: CH */
static const fw_rule r_ch[] = {
    { "", "AS", "+", "cAS" },
    { " ", "ALET", "", "caLA" },
    { "", "A", "LL", "ca" },
    { "", "AM", "ELEON", "KxM" },
    { "", "AMPAGNE", "", "saMPAN" },
    { "", "ANG", "+", "cANj" },
    { " ", "AO", "", "CeYQ" },
    { "", "APEL", "", "caP\300" },
    { "", "ARABANC", "", "saRxBan" },
    { "", "ARADE", "", "sxRhD" },
    { " ", "ARA", "", "6aRx" },
    { " ", "ARLIE", "", "ch|Lb" },
    { " ", "ARISMA", "", "6aRiZMa" },
    { "", "ASM", "", "6aZM" },
    { " ", "AU", "FF", "sO" },
    { " ", "AUVIN", "", "sOVxN" },
    { "", "EESE", "", "cEZ" },
    { " ", "DIR", "", "SEAc DEIh|" },
    { " ", "ELTEN", "", "ce1T7" },
    { " ", "ELSEA", "", "ce1SE" },
    { " ", "EMO", "", "CEMO" },
    { " ", "E", "M", "Ce" },
    { " ", "ILDREN", "", "ci1D37" },
    { " ", "ILE", "", "ciLb" },
    { " ", "IN", "#", "cIN" },
    { "#", "IN", "#", "sEN" },
    { "", "I", "ROPO", "ci" },
    { " ", "KDSK", " ", "cek DiSk" },
    { "", "LOR", "", "6Lo|" },
    { "", "OCOLA", "T", "cQ6Lx" },
    { "", "O", "K+", "cO" },
    { "", "O", "L", "6Q" },
    { "", "O", "R+", "6Q" },
    { "", "OIR", "", "6WIx|" },
    { "", "OR", "D", "6o" },
    { "", "OR", "#!", "6oR" },
    { " ", "RIST", "IAN", "KRiSf" },
    { " ", "RISTMAS", "", "KRiSMxS" },
    { " ", "ROM", "#", "KROM" },
    { "", "RI", "ST ", "KRI" },
    { "", "RIST", "EN", "KRiS" },
    { "", "ROM", "", "KROM" },
    { "", "RONI", "!#", "KRxNI" },
    { "", "RON", "", "KRQN" },
    { " ", "R", "", "KR" },
    { "", "UT", "E", "sUT" },
    { "!E", "", "", "K" },
    { "", "", "", "c" },
};
static const fw_rule_list l_ch = { r_ch, 48 };

/* lts_rules_c at $6C64: CI */
static const fw_rule r_ci[] = {
    { "", "", "ALI", "sb" },
    { "", "", "AL", "s" },
    { "", "AN", "", "sxN" },
    { "", "A", "TION", "SbA" },
    { "", "AT", "+", "sbAT" },
    { "", "", "A", "s" },
    { ":#:", "E", "D", "Sb" },
    { "", "", "EN", "s" },
    { "", "G", "AR", "SiG" },
    { "", "NEMA", "", "SiNiMx" },
    { "", "", "OL", "Sb" },
    { "", "", "O", "s" },
    { "", "PH", "", "SIF" },
    { "", "RENCE", "STER", "SIR7Se" },
    { "", "", "SION", "Si" },
    { "R", "S", "+", "SIZ" },
    { "", "S", "+", "SIS" },
    { "", "TIZ", "EN", "SiTiZ" },
    { " ", "", "T+", "Si" },
    { "", "VI", "LIZ", "SiVi" },
    { "", "VI", "LI", "SxVi" },
    { "", "V", "IE", "SiV" },
    { "", "VI", "", "SiVi" },
};
static const fw_rule_list l_ci = { r_ci, 23 };

/* lts_rules_c at $6D22: CK */
static const fw_rule r_ck[] = {
    { "", "", "+", "C" },
    { "", "", "#", "6" },
    { "", "", " ", "k" },
    { "", "", "", "K" },
};
static const fw_rule_list l_ck = { r_ck, 4 };

/* lts_rules_c at $6D32: CL */
static const fw_rule r_cl[] = {
    { "", "ARINE", "T", "6LaRiNe" },
    { "", "EANS", "", "6LeNZ" },
    { "", "EAR", "", "6LJ|" },
    { "", "ER", "K", "6Lh" },
    { "", "IEN", "T", "6LIxN" },
    { "", "I", "MB", "6LI" },
    { "", "I", "M#", "6LI" },
    { "", "OSE", "LY", "6LOS" },
    { "", "OS", "ER", "6LOS" },
    { "", "OSE", "T", "6LQSi" },
    { "", "OS", "% ", "6LOZ" },
    { "", "OV", "EN", "6LOV" },
    { "", "OWN", "", "6LwN" },
    { "", "YDE", "", "6LID" },
    { ":#:", "", "% ", "6\300" },
};
static const fw_rule_list l_cl = { r_cl, 15 };

/* lts_rules_c at $6DC8: CO */
static const fw_rule r_co[] = {
    { "", "A", "GU", "KOa" },
    { "", "COA", "", "KOKO" },
    { "", "CO", "", "KOKO" },
    { "", "INC", "", "KOinS" },
    { "", "LLAGE", "", "KQLhz" },
    { "", "LLE", "CT", "KxLe" },
    { "", "LLEG", "E", "KQLij" },
    { "", "LIS", "EUM", "KQLiS" },
    { "", "LON", ": ", "6OLxN" },
    { "", "LONE", "L", "KrNx" },
    { "", "LOGNE", "", "KxLON" },
    { "", "LOUR", "", "KuLx|" },
    { " ", "LUMB", "", "KxLuMB" },
    { "", "M", "BINE", "KxM" },
    { "", "MB", "% ", "KOM" },
    { "", "MB", ": ", "KOM" },
    { "", "MED", "I", "KxMED" },
    { "", "ME", "D", "KQMx" },
    { "", "MFOR", "T", "KuMFx" },
    { "", "M", "M#ND", "KxM" },
    { "", "MM", "IT", "KxM" },
    { "", "MMEN", "C+", "KxMeN" },
    { "", "MME", "NT", "6QMe" },
    { "", "MMON", "", "KQMxN" },
    { "", "MM", "UNIS", "KQM" },
    { "", "MM", "UN", "KxM" },
    { " ", "MPAN", "", "KuMPxN" },
    { "", "MPA", "SS", "KuMPx" },
    { "", "MPAT", "", "KQMPaT" },
    { "", "MPIL", "AT", "KQMPiL" },
    { "", "MPIL", "+", "KxMPIL" },
    { "", "M", "POSIT", "KQM" },
    { "", "M", "POS+", "KxM" },
    { "", "M", "PUT", "6QM" },
    { "", "MPLE", "T+", "6QMPLE" },
    { "", "MP", " ", "6QMp" },
    { "", "M", "%", "KuM" },
    { "", "NFER", "#", "6QNFx|" },
    { "", "NFIN", "", "6QNFIN" },
    { "", "NGEN", "", "6QNjEN" },
    { "", "N", "GLO", "6QN" },
    { "", "NN", "ECT", "KxN" },
    { "", "NSI", "D", "KxNSi" },
    { " ", "NT", "ACT", "KQNT" },
    { "", "NT", "A!:#", "KxNT" },
    { "", "N", "TENTED", "KxN" },
    { "", "NTEN", "T", "KQNTeN" },
    { "", "NTINEN", "T", "KQNTiNeN" },
    { "", "NTI", "N", "KxNTi" },
    { "", "NTR", "ACT", "6QNT3" },
    { "", "NTRA", "ST", "KxNT3h" },
    { "", "NTRA", "", "6QNT3x" },
    { "", "NTRIV", "", "6QNT3IV" },
    { "", "NTRO", "L", "KxNT3O" },
    { "", "NVEN", "IENT", "KxNVEN" },
    { "", "NVEN", "T", "KxNVeN" },
    { "", "", "OPERAT", "KO" },
    { "", "", "OPT", "KO" },
    { "", "", "ORD", "KO" },
    { "", "P", "%", "KOP" },
    { "", "P", "+", "6QP" },
    { "", "RPS", " ", "6o|" },
    { "", "RR", "ECT", "KxR" },
    { "", "RRESPO", "ND", "KQRxSpQ" },
    { "", "RR", "UPT", "KxR" },
    { "", "RPOR", "", "6oPx|" },
    { "", "R", "%", "6o|" },
    { "", "", "ST", "6Q" },
    { "", "SY", "", "KOZb" },
    { "", "UGH", "", "6QF" },
    { "", "UL", "D", "6v" },
    { "", "UNTR", "+", "KuNT3" },
    { "", "U", "PL", "Ku" },
    { "", "UR", "AG", "KuR" },
    { "", "URTE", "SY", "KrTx" },
    { "", "UR", "!", "Ko" },
    { " ", "USIN", "", "KuZ7" },
    { "", "U", "TH", "KU" },
    { "", "V", "ER", "KuV" },
};
static const fw_rule_list l_co = { r_co, 79 };

/* lts_rules_c at $7114: CP */
static const fw_rule r_cp[] = {
    { " ", "U", " ", "SEPEYU" },
};
static const fw_rule_list l_cp = { r_cp, 1 };

/* lts_rules_c at $7120: CQ */
static const fw_rule r_cq[] = {
    { "", "U", "", "KW" },
};
static const fw_rule_list l_cq = { r_cq, 1 };

/* lts_rules_c at $7126: CR */
static const fw_rule r_cr[] = {
    { "", "A", "DLE", "KRA" },
    { "", "AGGE", "D", "KRaGi" },
    { "", "EAT", "OR", "KRbAT" },
    { "", "EA", "T+", "KREA" },
    { "", "E", "MENT", "KRx" },
    { "", "IM", "EA", "KRIM" },
    { "", "IMIN", "", "KRiMiN" },
    { "", "OA", "T: ", "KROa" },
    { "", "OATIA", "", "KROAsx" },
    { "", "OCHET", "", "KROsA" },
    { "", "OOKE", "D", "KRvKi" },
    { "", "OW", "D", "KRw" },
    { "", "OW", "", "KRO" },
    { "", "Y", " ", "KRI" },
    { "", "", "", "KR" },
};
static const fw_rule_list l_cr = { r_cr, 15 };

/* lts_rules_c at $71BC: CT */
static const fw_rule r_ct[] = {
    { " ", "TY", " ", "SETETEWI" },
    { "", "", " ", "KT" },
};
static const fw_rule_list l_ct = { r_ct, 2 };

/* lts_rules_c at $71D0: CU */
static const fw_rule r_cu[] = {
    { "!", "I", "TOUS", "KYUi" },
    { "!", "IT", "", "KiT" },
    { "", "LMIN", "#", "Ku1MiN" },
    { "", "PBOAR", "D", "KuBx" },
    { "", "R", "+", "CYq|" },
    { "", "SH", "ION", "Kvs" },
    { " ", "STOM", "", "KuSTxM" },
    { "", "TO", "FF", "KuTQ" },
};
static const fw_rule_list l_cu = { r_cu, 8 };

/* lts_rules_c at $7227: CY */
static const fw_rule r_cy[] = {
    { "", "L", "", "SiL" },
    { "", "", "CL", "SI" },
};
static const fw_rule_list l_cy = { r_cy, 2 };

/* lts_rules_c at $7234: CZ */
static const fw_rule r_cz[] = {
    { " ", "ECHOSLOVA", "K", "cekOSLOVa" },
    { " ", "ECH", "", "cek" },
};
static const fw_rule_list l_cz = { r_cz, 2 };

static const fw_rule_list *const k_lts_rules_c[] = {
    &l_ca, &l_cc, &l_ce, &l_ch, &l_ci, &l_ck,
    &l_cl, &l_co, &l_cp, &l_cq, &l_cr, &l_ct,
    &l_cu, &l_cy, &l_cz,
};
const fw_rule_table lts_rules_c = { "lts_rules_c", 0x675D, 2807, false, k_lts_rules_c, 15 };

const fw_rule_list *const lts_pairs_c[26] = {
    ['A' - 'A'] = &l_ca,
    ['C' - 'A'] = &l_cc,
    ['E' - 'A'] = &l_ce,
    ['H' - 'A'] = &l_ch,
    ['I' - 'A'] = &l_ci,
    ['K' - 'A'] = &l_ck,
    ['L' - 'A'] = &l_cl,
    ['O' - 'A'] = &l_co,
    ['P' - 'A'] = &l_cp,
    ['Q' - 'A'] = &l_cq,
    ['R' - 'A'] = &l_cr,
    ['T' - 'A'] = &l_ct,
    ['U' - 'A'] = &l_cu,
    ['Y' - 'A'] = &l_cy,
    ['Z' - 'A'] = &l_cz,
};

/* lts_rules_d at $7254: DA */
static const fw_rule r_da[] = {
    { "", "M", "AG%", "DaM" },
    { " ", "NG", "ER", "DANj" },
    { " ", "N", "I", "DAN" },
    { "", "R", "%", "Dl|" },
    { " ", "TA", "", "DATx" },
    { "", "Y", "", "DA" },
};
static const fw_rule_list l_da = { r_da, 6 };

/* lts_rules_d at $7286: DE */
static const fw_rule r_de[] = {
    { "", "A", "F", "De" },
    { "", "", "ALL", "DE" },
    { "", "A", "LT", "De" },
    { "", "ARTH", "", "Drt" },
    { "", "AR", "", "DJ|" },
    { "", "ATH", "", "Det" },
    { "", "B", "T", "De" },
    { "", "", "CEN!", "DE" },
    { " ", "C", " ", "Dek" },
    { "", "COR", ": ", "De6o|" },
    { "", "C", "EMBER", "DiS" },
    { "", "CIM", "", "DeSiM" },
    { "", "", "CI", "Di" },
    { "", "CR", "EE", "DikR" },
    { "", "EP", "!", "DEP" },
    { "", "FEN", "", "DiFeN" },
    { "", "FIN", "AB", "DiFIN" },
    { "", "FIN", "IT+", "DeFiN" },
    { "", "FIN", "+: ", "DiFIN" },
    { "", "FE", "CT", "DEFe" },
    { " ", "", "F", "Di" },
    { "", "FY", "", "DiFI" },
    { "", "", "GRAD", "Di" },
    { "", "", "GREE", "Di" },
    { "", "", "LAY", "Di" },
    { "", "", "LET", "Di" },
    { "", "LIV", "ER", "DiLiV" },
    { "", "", "MAND", "Di" },
    { "", "MON", "ST", "DeMxN" },
    { "", "MON", "", "DEMxN" },
    { "", "MOR", "", "DiMQ|" },
    { "", "M", "OT", "DiM" },
    { "", "M", "U", "DiM" },
    { "", "M", "O", "DeM" },
    { " ", "NY", "", "DiNI" },
    { " ", "NI", "#: ", "DiNI" },
    { "", "NOMIN", "AT", "DiNQMiN" },
    { "", "", "PEND", "Di" },
    { "", "", "PRESS", "Di" },
    { "", "PRIV", "+", "DiPRIV" },
    { " ", "RB", "Y", "DhB" },
    { "", "RIV", "#: ", "DiRIV" },
    { "", "SCRI", "B", "DiSKRI" },
    { "", "", "SCRIP", "Di" },
    { "", "", "SEGRE", "DE" },
    { "", "S", "IGNA", "DeZ" },
    { "", "S", "IGN", "DiZ" },
    { "", "S", "IR", "DiZ" },
    { "", "SPI", "!+", "DiSpI" },
    { "", "", "STIN", "De" },
    { "", "", "ST", "Di" },
    { "", "TAI", "L", "DETA" },
    { "", "", "TECT", "Di" },
    { "", "TERMIN", "", "DiTrMiN" },
    { "", "", "VOT+", "Di" },
    { "", "VELO", "P", "DiVeLx" },
    { "", "VI", "!#", "DiVI" },
    { "", "W", "", "DYU" },
    { " ", "", "!:#", "De" },
};
static const fw_rule_list l_de = { r_de, 59 };

/* lts_rules_d at $74AD: DG */
static const fw_rule r_dg[] = {
    { "", "", "MENT", "j" },
    { "", "", "+", "j" },
};
static const fw_rule_list l_dg = { r_dg, 2 };

/* lts_rules_d at $74B9: DI */
static const fw_rule r_di[] = {
    { " ", "ATOM", " ", "DIa" },
    { " ", "A", "MET", "{DI}a" },
    { " ", "A", "!", "DIx" },
    { "", "E", "T", "DIx" },
    { " ", "FFER", "", "DiFx|" },
    { " ", "FFI", "", "DiFi" },
    { "", "GM", "", "DIM" },
    { " ", "L", "IG", "DiL" },
    { "", "MIN", "", "DiMiN" },
    { " ", "R", " ", "DEIh|" },
    { " ", "SA", "BL#", "DiSA" },
    { "", "SCIP", "LE", "DiSIP" },
    { " ", "SCOMP", " ", "DiSKQMp" },
    { "", "SH", "%", "Dis" },
    { "", "SH", " ", "Dis" },
    { " ", "S", "PUT", "DiS" },
    { "", "SSOLVE", "", "DiZQ1V" },
    { "", "STAN", "", "DiSTxN" },
    { " ", "S", "", "DiS" },
    { "", "VI", "DEND", "DiVi" },
    { "", "V", "%", "DIV" },
    { "", "VINE", "", "DiVIN" },
    { "", "V", "I", "DiV" },
};
static const fw_rule_list l_di = { r_di, 23 };

/* lts_rules_d at $75A3: DJ */
static const fw_rule r_dj[] = {
    { "", "", "", "j" },
};
static const fw_rule_list l_dj = { r_dj, 1 };

/* lts_rules_d at $75A7: DO */
static const fw_rule r_do[] = {
    { " ", "", " ", "DU" },
    { " ", "E", "S", "Du" },
    { "", "LLARS", "", "DQLrZ" },
    { "", "", "LL", "DQ" },
    { "", "M", "EST", "DxM" },
    { " ", "MINO", "", "DQMiNO" },
    { "#:", "M", ": ", "DxM" },
    { " ", "NE", "", "DuN" },
    { " ", "OR", "", "Do|" },
    { "", "UGH", "", "DO" },
    { "", "UB", "T", "Dw" },
    { " ", "S", " ", "DQS" },
    { "", "WN", "", "DwN" },
    { "", "W", "", "DO" },
    { "", "CTRIN", "", "DQKT3iN" },
    { "", "Z", "EN", "DuZ" },
    { " ", "", "%", "DU" },
};
static const fw_rule_list l_do = { r_do, 17 };

/* lts_rules_d at $7635: DR */
static const fw_rule r_dr[] = {
    { "", "ESSAG", "+", "D3eShz" },
    { "", "IV", "%", "D3IV" },
    { "", "IV", "E!", "D3iV" },
    { "", "AMA", "TIC", "D3xMa" },
    { " ", "AMA", ": ", "D3hMx" },
    { " ", "AUGH", "T", "D3hF" },
    { "", "OUGH", "T", "D3w" },
    { "", "OVE", "", "D3OV" },
};
static const fw_rule_list l_dr = { r_dr, 8 };

/* lts_rules_d at $7690: DU */
static const fw_rule r_du[] = {
    { "", "", "A", "jU" },
    { "", "E", "", "DYU" },
    { "", "GOU", "T", "DuGw" },
    { "", "", "PL+", "DYU" },
    { "", "R", "HAM", "DuR" },
    { "#:#:", "RE", "", "jq|" },
    { " :#:", "R", "% ", "DYq|" },
};
static const fw_rule_list l_du = { r_du, 7 };

/* lts_rules_d at $76CF: DY */
static const fw_rule r_dy[] = {
    { "", "NAMITE", "", "DINxMIT" },
};
static const fw_rule_list l_dy = { r_dy, 1 };

static const fw_rule_list *const k_lts_rules_d[] = {
    &l_da, &l_de, &l_dg, &l_di, &l_dj, &l_do,
    &l_dr, &l_du, &l_dy,
};
const fw_rule_table lts_rules_d = { "lts_rules_d", 0x7254, 1163, false, k_lts_rules_d, 9 };

const fw_rule_list *const lts_pairs_d[26] = {
    ['A' - 'A'] = &l_da,
    ['E' - 'A'] = &l_de,
    ['G' - 'A'] = &l_dg,
    ['I' - 'A'] = &l_di,
    ['J' - 'A'] = &l_dj,
    ['O' - 'A'] = &l_do,
    ['R' - 'A'] = &l_dr,
    ['U' - 'A'] = &l_du,
    ['Y' - 'A'] = &l_dy,
};

/* lts_rules_e at $76DF: EA */
static const fw_rule r_ea[] = {
    { "", "B", "LE", "xB" },
    { "", "CH", "", "Ec" },
    { "", "", "CT", "ba" },
    { "", "", "D", "e" },
    { "", "G", "ER", "EG" },
    { "", "", "LOUS", "e" },
    { "#:", "N", " ", "JN" },
    { "#:", "NS", " ", "JNZ" },
    { "", "R", "LO", "J" },
    { "", "RL", "", "rL" },
    { "", "RN", "", "rN" },
    { "", "RTH", "", "rt" },
    { "", "R", "", "J|" },
    { "", "", "SURE", "e" },
    { "", "", "SU", "e" },
    { "#:", "S", " ", "JZ" },
    { ":!", "THE", " ", "Ed" },
    { ":!", "TH", "", "et" },
    { "", "VEN", "", "eV7" },
    { "", "U", "", "O" },
    { ":#:", "", " ", "J" },
    { "", "", "", "E" },
};
static const fw_rule_list l_ea = { r_ea, 22 };

/* lts_rules_e at $7771: EC */
static const fw_rule r_ec[] = {
    { " ", "HO", "", "eKO" },
    { " ", "O", "!O!IC", "EKx" },
    { " ", "O", "!O!", "i6Q" },
    { " ", "O", "", "E6O" },
    { " ", "UADOR", "", "e6WxDo|" },
};
static const fw_rule_list l_ec = { r_ec, 5 };

/* lts_rules_e at $77A6: ED */
static const fw_rule r_ed[] = {
    { " ", "", " ", "eD" },
    { "", "G", "L", "ej" },
    { "", "GE", "", "ej" },
    { "", "G", "+", "ej" },
    { "", "IA", "TE", "EDbx" },
    { " ", "I", "T", "eDi" },
    { " :#:!#:!", "", "LY", "iD" },
    { " :#:!", "", "LY", "iD" },
    { ":!", "", "LY", "D" },
    { "#", "NESS", "", "DNxS" },
    { "", "NESS", "", "iDNxS" },
    { ":#:!", "", "Y ", "xD" },
    { " #:B", "", "", "eD" },
    { " :!", "", "", "eD" },
    { "#GH", "", "", "D" },
    { "#:D", "", "", "iD" },
    { "#:T", "", "", "iD" },
    { ":#:.", "", " ", "D" },
    { ":#RR", "", " ", "D" },
    { ":#R", "", " ", "D" },
    { ":#W", "", " ", "D" },
    { ":#", "", " ", "D" },
    { ":#:!", "", " ", "T" },
    { "", "", " ", "D" },
    { " ", "", "U", "ej" },
    { " ", "", "", "eD" },
};
static const fw_rule_list l_ed = { r_ed, 26 };

/* lts_rules_e at $786F: EE */
static const fw_rule r_ee[] = {
    { "", "R", "", "J|" },
    { "", "S", " ", "EZ" },
    { "", "", "", "E" },
};
static const fw_rule_list l_ee = { r_ee, 3 };

/* lts_rules_e at $787E: EF */
static const fw_rule r_ef[] = {
    { " ", "F", "ECT", "iF" },
    { "", "FOR", "T", "eFx" },
    { "", "U", "L", "Fv" },
};
static const fw_rule_list l_ef = { r_ef, 3 };

/* lts_rules_e at $7897: EG */
static const fw_rule r_eg[] = {
    { " ", "", " ", "EjE" },
    { "", "", "IC", "Ej" },
    { "", "YP", "T", "EjiP" },
};
static const fw_rule_list l_eg = { r_eg, 3 };

/* lts_rules_e at $78AE: EH */
static const fw_rule r_eh[] = {
    { "", "OO", "D", "Hv" },
};
static const fw_rule_list l_eh = { r_eh, 1 };

/* lts_rules_e at $78B6: EI */
static const fw_rule r_ei[] = {
    { "H", "GH", "", "I" },
    { "", "GHTH", "", "ATt" },
    { "", "THER", "", "Idx|" },
    { "", "T", "Y", "AxT" },
    { "", "", "", "A" },
};
static const fw_rule_list l_ei = { r_ei, 5 };

/* lts_rules_e at $78DA: EL */
static const fw_rule r_el[] = {
    { "", "ECTROLY", "", "iLeKT3xLI" },
    { "", "ECTRO", "N", "iLeKT3Q" },
    { "", "ECTRO", "", "iLeKT3O" },
    { "", "EC", "T", "iLeK" },
    { " ", "E", "MENT", "eLx" },
    { "#:", "ESS", "", "LxS" },
    { " ", "", "EVEN", "iL" },
    { "", "IA", "N", "eLIx" },
    { "#:", "", "%", "L" },
    { "", "ITE", "", "eLET" },
    { "", "IZAB", "", "bLiZxB" },
    { " ", "", "ONG", "EL" },
    { "", "SE", "", "~eLS" },
    { "#:", "", "Y ", "L" },
    { " ", "", "", "eL" },
};
static const fw_rule_list l_el = { r_el, 15 };

/* lts_rules_e at $7971: EM */
static const fw_rule r_em[] = {
    { "!#:", "EN", "T", "MxN" },
    { " ", "ER", "ALD", "eMxR" },
    { " ", "ERG", "", "iMrj" },
    { "", "E", ": ", "EM" },
    { " ", "", "OT", "iM" },
    { " ", "PHAS", "", "eMFxS" },
};
static const fw_rule_list l_em = { r_em, 6 };

/* lts_rules_e at $79AD: EN */
static const fw_rule r_en[] = {
    { "", "", "ABLE", "iN" },
    { "", "AMEL", "", "iNaM\300" },
    { "#:", "", "C%", "xN" },
    { "#:", "", "C+", "eN" },
    { "", "CH", "", "eNc" },
    { "", "CYCLOPE", "DIA", "eNSIKLOPE" },
    { "#:!", "", "ED", "xN" },
    { "", "EM", "", "eNxM" },
    { "", "ERGE", "TIC", "eNxje" },
    { "", "ERG", "#", "eNxj" },
    { "#:!", "", "ER", "xN" },
    { "#:", "ESS", "", "NxS" },
    { "", "GAG", "", "inGAj" },
    { "", "GIN", "", "eNjiN" },
    { " ", "", "GR#", "iN" },
    { "", "G", "L", "iNG" },
    { " ", "", "ORM", "iN" },
    { " ", "", "OU", "iN" },
    { "", "T", "IR", "iNT" },
    { "", "TITL", "", "iNTIT\300" },
    { "", "TR", "IES", "eNT3" },
    { "", "TR", "Y", "eNT3" },
    { "", "VIRON", "", "iNVIRxN" },
    { "#:!", "", "%", "xN" },
    { "#:!", "S", " ", "xNZ" },
    { "!#:", "", "TAL", "eN" },
    { "!#:", "", "TATION", "eN" },
    { "!#:", "", "TI#", "eN" },
    { "!#:", "", "TED", "eN" },
    { "!#:", "", "TING", "eN" },
    { "!#:", "", "T", "xN" },
    { "#:", "", " ", "xN" },
};
static const fw_rule_list l_en = { r_en, 32 };

/* lts_rules_e at $7AE8: EO */
static const fw_rule r_eo[] = {
    { " ", "F", " ", "EOeF" },
    { "G", "IS", " ", "Wh" },
    { "G", "ISE", " ", "WhZ" },
    { "G", "N", "", "xN" },
    { ":", "N", "", "EQN" },
    { "", "N", "", "iN" },
    { "", "PARD", "", "ePxD" },
    { "", "US", "", "bxS" },
};
static const fw_rule_list l_eo = { r_eo, 8 };

/* lts_rules_e at $7B27: EP */
static const fw_rule r_ep[] = {
    { " ", "I", "", "ePi" },
};
static const fw_rule_list l_ep = { r_ep, 1 };

/* lts_rules_e at $7B2F: EQ */
static const fw_rule r_eq[] = {
    { "", "UA", "L", "E6Wx" },
    { " ", "UA", "T", "{i6WA" },
    { "", "UIP", "", "{i6WiP" },
    { "", "UIVAL", "EN", "{i6WiVxL" },
};
static const fw_rule_list l_eq = { r_eq, 4 };

/* lts_rules_e at $7B60: ER */
static const fw_rule r_er[] = {
    { " ", "A", ": ", "JRx" },
    { "", "AR", "Y", "xR" },
    { "", "", "ASE", "iR" },
    { "", "AR", "IES", "xR" },
    { "", "", "ENC", "x|" },
    { "", "E", " ", "J|" },
    { "", "E", "LY ", "J|" },
    { "!", "", "FUL", "x" },
    { "", "", "IC", "eR" },
    { "!", "", "IES", "xR" },
    { "", "I", "MENT", "eRi" },
    { "", "", "ING", "xR" },
    { "#:", "", "ISTIC", "xR" },
    { " ", "ITREA", "", "eRiT3Ax" },
    { "", "", "ITY", "eR" },
    { " ", "", "#", "i|" },
    { "", "I", "#", "ERE" },
    { "!", "R", "ATION", "xR" },
    { "!", "RE", "D", "rD" },
    { "", "ROR", "", "eRx|" },
    { " ", "R", "#", "eR" },
    { " ", "", "!", "r|" },
    { "!", "", "S ", "x" },
    { " :!", "R", "", "eR" },
    { "!", "", "!", "r" },
    { "", "", "", "x|" },
};
static const fw_rule_list l_er = { r_er, 26 };

/* lts_rules_e at $7C2A: ES */
static const fw_rule r_es[] = {
    { " ", "CA", "P+", "iSKA" },
    { " ", "CAP", "#", "eSKxP" },
    { "", "C", "ENCE", "eS" },
    { "", "C", "ENT", "eS" },
    { "", "C", "% ", "eS" },
    { "", "ENCE", "", "eZ7S" },
    { "", "E", " ", "EZ" },
    { "", "", "IA", "ES" },
    { "", "", "ION", "Ez" },
    { "", "QU", "% ", "eSk" },
    { "", "QU", "+", "eSC" },
    { " ", "S", "ENT", "iS" },
    { ":#:TR", "S", "% ", "xS" },
    { "", "S", "ION", "es" },
    { "", "S", "%", "eS" },
    { "", "TA", "BLISH", "iSfa" },
    { "#:!", "T", " ", "iSf" },
    { "#:C", "", " ", "iZ" },
    { "#:&", "", " ", "iZ" },
};
static const fw_rule_list l_es = { r_es, 19 };

/* lts_rules_e at $7CD2: ET */
static const fw_rule r_et[] = {
    { " ", "C", " ", "eT SeTxRx" },
    { "#:!", "", "E: ", "ET" },
    { "", "H", "AN ", "Et" },
    { " ", "H", "ER ", "Et" },
    { " ", "HER", "+", "btJ" },
    { "", "H", "IC", "et" },
    { " ", "HIO", "P", "EtbO" },
    { "CK", "", ": ", "iT" },
    { "#:", "", "IC", "eT" },
    { "#:!", "", "ION", "Es" },
    { "#:!", "R", "Y", "xT3" },
    { "#!#!", "", "% ", "ET" },
    { "", "T", "E", "e{T" },
};
static const fw_rule_list l_et = { r_et, 13 };

/* lts_rules_e at $7D4D: EU */
static const fw_rule r_eu[] = {
    { " ", "ROPA", " ", "YvROPx" },
    { " ", "RO", "P", "YqRx" },
    { " ", "RO", " ", "YqRO" },
    { ":#:", "M", "", "bxM" },
    { "", "R", "", "r|" },
    { "", "SE", ": ", "rZ" },
    { ":#:", "S", " ", "JS" },
    { "@", "", "", "U" },
    { "", "", "", "YU" },
};
static const fw_rule_list l_eu = { r_eu, 9 };

/* lts_rules_e at $7D97: EV */
static const fw_rule r_ev[] = {
    { " ", "AS", " ", "EVEAeS" },
    { " ", "E", "NT", "iVe" },
    { " UN", "EN", "", "EVxN" },
    { " ", "EN", "", "EVxN" },
    { " ", "ER", "Y", "eVR" },
    { "", "", "ER", "eV" },
    { " ", "I", "D", "eVi" },
    { " ", "I", "L", "EVx" },
};
static const fw_rule_list l_ev = { r_ev, 8 };

/* lts_rules_e at $7DE0: EW */
static const fw_rule r_ew[] = {
    { "@", "", "", "U" },
    { "", "", "", "YU" },
};
static const fw_rule_list l_ew = { r_ew, 2 };

/* lts_rules_e at $7DE9: EX */
static const fw_rule r_ex[] = {
    { "", "AGG", "", "iGZaj" },
    { "", "AMIN", "#", "iGZaMiN" },
    { "", "A", "MPL", "iGZh" },
    { " ", "", "A", "iGZ" },
    { "", "A", "", "ekSa" },
    { "", "CA", "V#", "ekSKx" },
    { "", "C", "EPT", "ikS" },
    { "", "CI", "TATION", "ekSi" },
    { "", "CI", "T", "ikSI" },
    { " ", "C", "+", "ekS" },
    { " ", "ECU", "TIVE", "{iGZeKYu" },
    { "", "ECU", "T", "ekSeKYU" },
    { " ", "E", " ", "eKSb" },
    { " ", "E2BIN", " ", "ekSb TU BiN" },
    { " ", "E", "C ", "iGZe" },
    { "", "H", "AUST", "iGZ" },
    { "", "HIB", "ITION", "ekSiB" },
    { "", "HIBIT", "", "iGZiBiT" },
    { " ", "I", "T", "ekSi" },
    { " ", "", "I", "iGZ" },
    { " ", "", "O", "ekS" },
    { "", "P", "ERT", "ekSp" },
    { "", "PLAN", "A", "ekSPLxN" },
    { "", "PLOI", "T", "ekSPLy" },
    { "", "PO", "S", "ekSpO" },
    { "", "", "P", "ikS" },
    { " ", "TEN", "", "ikSfeN" },
    { "", "TREM", "#", "ikSf3EM" },
    { " ", "", "U", "iGZ" },
};
static const fw_rule_list l_ex = { r_ex, 29 };

/* lts_rules_e at $7F27: EY */
static const fw_rule r_ey[] = {
    { "", "E", "", "I" },
    { " :", "", " ", "E" },
    { "", "", " ", "b" },
    { "", "S", " ", "EZ" },
    { "K", "", "", "E" },
    { "", "", "", "A" },
};
static const fw_rule_list l_ey = { r_ey, 6 };

static const fw_rule_list *const k_lts_rules_e[] = {
    &l_ea, &l_ec, &l_ed, &l_ee, &l_ef, &l_eg,
    &l_eh, &l_ei, &l_el, &l_em, &l_en, &l_eo,
    &l_ep, &l_eq, &l_er, &l_es, &l_et, &l_eu,
    &l_ev, &l_ew, &l_ex, &l_ey,
};
const fw_rule_table lts_rules_e = { "lts_rules_e", 0x76DF, 2148, false, k_lts_rules_e, 22 };

const fw_rule_list *const lts_pairs_e[26] = {
    ['A' - 'A'] = &l_ea,
    ['C' - 'A'] = &l_ec,
    ['D' - 'A'] = &l_ed,
    ['E' - 'A'] = &l_ee,
    ['F' - 'A'] = &l_ef,
    ['G' - 'A'] = &l_eg,
    ['H' - 'A'] = &l_eh,
    ['I' - 'A'] = &l_ei,
    ['L' - 'A'] = &l_el,
    ['M' - 'A'] = &l_em,
    ['N' - 'A'] = &l_en,
    ['O' - 'A'] = &l_eo,
    ['P' - 'A'] = &l_ep,
    ['Q' - 'A'] = &l_eq,
    ['R' - 'A'] = &l_er,
    ['S' - 'A'] = &l_es,
    ['T' - 'A'] = &l_et,
    ['U' - 'A'] = &l_eu,
    ['V' - 'A'] = &l_ev,
    ['W' - 'A'] = &l_ew,
    ['X' - 'A'] = &l_ex,
    ['Y' - 'A'] = &l_ey,
};

/* lts_rules_f at $7F43: FA */
static const fw_rule r_fa[] = {
    { "", "", "BUL", "Fa" },
    { " ", "C", "U", "Fa6" },
    { "", "CADE", "", "FxShD" },
    { "", "CE", "T", "FaSi" },
    { "", "CI", "LE", "FaSI" },
    { "", "C", "ILI", "FxS" },
    { "", "HREN", "", "FhRxN" },
    { "", "M", "ILIA", "FxM" },
    { "", "M", "OUS", "FAM" },
    { "", "NT", "AS", "FaNT" },
    { "", "VOUR", "", "FAVx|" },
    { "", "TI", "GU", "FxTE" },
};
static const fw_rule_list l_fa = { r_fa, 12 };

/* lts_rules_f at $7FB7: FD */
static const fw_rule r_fd[] = {
    { " ", "IS", "! ", "eFDiS" },
};
static const fw_rule_list l_fd = { r_fd, 1 };

/* lts_rules_f at $7FC4: FE */
static const fw_rule r_fe[] = {
    { "", "", "DER", "Fe" },
    { "", "", "LIN", "FE" },
    { "", "", "MALE", "FE" },
    { "", "", "MUR", "FE" },
    { "PRE", "R", " ", "Fr|" },
    { "TRANS", "RR", "", "Fr|" },
    { "IN", "R", "", "Fr|" },
    { "", "", "SS", "Fe" },
    { " ", "V", "ER", "FEV" },
};
static const fw_rule_list l_fe = { r_fe, 9 };

/* lts_rules_f at $800F: FF */
static const fw_rule r_ff[] = {
    { "", "", "Y ", "F" },
};
static const fw_rule_list l_ff = { r_ff, 1 };

/* lts_rules_f at $8015: FI */
static const fw_rule r_fi[] = {
    { "", "A", "N", "FIa" },
    { "", "E", "D", "FI" },
    { "", "ES", "", "FIZ" },
    { "", "", "GUR", "Fi" },
    { " ", "J", "I", "FEj" },
    { "", "", "LE", "FI" },
    { "", "NGER", "", "FinGx|" },
    { "", "NI", "!!", "FiNi" },
    { "", "NI", "TE", "FINI" },
    { "", "NALE", "", "FiNhLb" },
    { "", "N", "AL", "FIN" },
    { "", "", "NANCE", "FI" },
    { "", "", "NANCI", "Fi" },
    { "", "RE", ":", "FIx|" },
};
static const fw_rule_list l_fi = { r_fi, 14 };

/* lts_rules_f at $808E: FL */
static const fw_rule r_fl[] = {
    { "", "A", "VOU", "FLA" },
    { "", "E", "CT", "FLe" },
    { "", "IES", " ", "FLIZ" },
    { "", "OO", "D", "FLu" },
    { "", "OOR", "", "FLo|" },
    { "", "OR", "#", "FLQR" },
    { "", "OUR", "", "FLwx|" },
    { "", "OW", "ER", "FLw" },
    { "", "UID", "", "FLUiD" },
    { "", "Y", " ", "FLI" },
};
static const fw_rule_list l_fl = { r_fl, 10 };

/* lts_rules_f at $80E8: FO */
static const fw_rule r_fo[] = {
    { "", "CUS", "", "FOKuS" },
    { "", "", "C", "FO" },
    { "", "", "NT", "FQ" },
    { " ", "R", " ", "~Fo|" },
    { "", "REIGN", "", "FQRiN" },
    { "", "RENS", "IC", "FxReNS" },
    { "", "REV", "ER", "FxReV" },
    { "", "RE", "STALL", "Fo|" },
    { "", "RE", "ST", "FQRi" },
    { "", "RE", "", "Fo|" },
    { "", "RFEI", "T", "FoFi" },
    { "", "RG", "#V", "FoG" },
    { "", "RG", "#T", "FoG" },
    { "", "RWAR", "D", "FoWx" },
    { "", "UR", "", "Fo|" },
};
static const fw_rule_list l_fo = { r_fo, 15 };

/* lts_rules_f at $8179: FR */
static const fw_rule r_fr[] = {
    { "", "A", "GR", "FRA" },
    { "", "AM", "+", "FRAM" },
    { "", "EQU", "", "FRE6W" },
    { "", "I", "DAY", "FRI" },
    { "", "IE", "ND", "FRe" },
    { " ", "OM", " ", "~FRQM" },
    { "", "O", "NT", "FRu" },
    { "", "O", "ST", "FRQ" },
    { "", "UI", "T", "FRU" },
};
static const fw_rule_list l_fr = { r_fr, 9 };

/* lts_rules_f at $81CA: FU */
static const fw_rule r_fu[] = {
    { "", "SELAG", "+", "FYUZxLhz" },
    { "", "", "RR+", "Fr" },
    { " ", "RTH", "ERMORE", "Frd" },
    { "", "", "L", "Fv" },
};
static const fw_rule_list l_fu = { r_fu, 4 };

/* lts_rules_f at $81F6: FY */
static const fw_rule r_fy[] = {
    { "+", "", " ", "FI" },
};
static const fw_rule_list l_fy = { r_fy, 1 };

static const fw_rule_list *const k_lts_rules_f[] = {
    &l_fa, &l_fd, &l_fe, &l_ff, &l_fi, &l_fl,
    &l_fo, &l_fr, &l_fu, &l_fy,
};
const fw_rule_table lts_rules_f = { "lts_rules_f", 0x7F43, 698, false, k_lts_rules_f, 10 };

const fw_rule_list *const lts_pairs_f[26] = {
    ['A' - 'A'] = &l_fa,
    ['D' - 'A'] = &l_fd,
    ['E' - 'A'] = &l_fe,
    ['F' - 'A'] = &l_ff,
    ['I' - 'A'] = &l_fi,
    ['L' - 'A'] = &l_fl,
    ['O' - 'A'] = &l_fo,
    ['R' - 'A'] = &l_fr,
    ['U' - 'A'] = &l_fu,
    ['Y' - 'A'] = &l_fy,
};

/* lts_rules_g at $81FD: GA */
static const fw_rule r_ga[] = {
    { " ", "BON", " ", "GaBQN" },
    { " ", "LA", ": ", "GhLx" },
    { " ", "LLER", "+", "GaLxR" },
    { "", "ME", "", "GAM" },
    { "", "RAG", "+", "GaRhz" },
    { "", "RE", "TH", "GaRx" },
    { "", "TE", "", "GAT" },
    { "", "TH", "ER", "Gad" },
    { "", "UGE", "", "GAj" },
    { " ", "S", " ", "GaS" },
};
static const fw_rule_list l_ga = { r_ga, 10 };

/* lts_rules_g at $825E: GE */
static const fw_rule r_ge[] = {
    { " ", "AR", "", "GJ|" },
    { "", "ES", "E", "GES" },
    { "", "NRE", "", "zQNRx" },
    { " ", "N", "ER", "jeN" },
    { " ", "N", "I", "jEN" },
    { "", "N", "ESIS", "jeN" },
    { "", "OFFR", "", "jeFR" },
    { "", "ORG", "", "joj" },
    { " ", "RM", "AN", "jrM" },
    { "", "", "T", "Ge" },
};
static const fw_rule_list l_ge = { r_ge, 10 };

/* lts_rules_g at $82B5: GG */
static const fw_rule r_gg[] = {
    { "", "", "", "G" },
};
static const fw_rule_list l_gg = { r_gg, 1 };

/* lts_rules_g at $82B9: GH */
static const fw_rule r_gh[] = {
    { " ", "ANA", ": ", "GhNx" },
    { " ", "ANAIAN", "", "GhNAxN" },
    { "", "O", "ST", "GO" },
    { "", "", "#ST", "G" },
    { " ", "", "", "G" },
    { "I", "", "", "" },
    { "U", "", "", "" },
};
static const fw_rule_list l_gh = { r_gh, 7 };

/* lts_rules_g at $82EC: GI */
static const fw_rule r_gi[] = {
    { "", "ANCE", " ", "jxNS" },
    { "", "AN", " ", "jxN" },
    { "", "ANS", " ", "jxNZ" },
    { "", "A", "N", "jIx" },
    { " ", "BB", "E", "jiB" },
    { "", "N", "E", "jiN" },
    { " ", "N", "", "jiN" },
    { ":#", "ON", "", "jxN" },
    { "", "", "OUS", "j" },
    { " ", "", "P", "ji" },
    { "", "", "RAFFE", "ji" },
    { " ", "", "ST", "ji" },
    { "", "TIS", "", "jITiS" },
    { "", "V", "", "GiV" },
};
static const fw_rule_list l_gi = { r_gi, 14 };

/* lts_rules_g at $835D: GL */
static const fw_rule r_gl[] = {
    { "", "ASG", "OW", "GLhZG" },
    { "", "OB", "#: ", "GLOB" },
    { "", "OM", "#", "GLQM" },
    { "", "OU", "CEST", "GLQ" },
};
static const fw_rule_list l_gl = { r_gl, 4 };

/* lts_rules_g at $8389: GM */
static const fw_rule r_gm[] = {
    { "#", "", "#", "GM" },
    { "", "", "", "M" },
};
static const fw_rule_list l_gm = { r_gm, 2 };

/* lts_rules_g at $8393: GN */
static const fw_rule r_gn[] = {
    { "", "EISS", "", "NIS" },
    { "!EI", "", "", "N" },
    { " ", "", "", "N" },
};
static const fw_rule_list l_gn = { r_gn, 3 };

/* lts_rules_g at $83A7: GO */
static const fw_rule r_go[] = {
    { "", "V", "ER", "GuV" },
    { "", "", "+", "GO" },
};
static const fw_rule_list l_go = { r_go, 2 };

/* lts_rules_g at $83B5: GR */
static const fw_rule r_gr[] = {
    { "", "ADU", "A", "GRaDYU" },
    { " ", "ANT", "", "GRhNT" },
    { "", "A", "PHIC", "GRa" },
    { "", "A", "PH", "GRh" },
    { "", "ATE", "FULL", "GRAT" },
    { "", "AVE", "L ", "GRaVx" },
    { "", "AVE", " ACCENT", "GRhV" },
    { "", "AVE", "", "GRAV" },
    { "", "EA", "T", "GRA" },
    { "", "E", "SS", "GRe" },
    { " ", "EY", "", "GRA" },
    { "", "ID", "", "GRiD" },
    { "", "IM", "+", "GRIM" },
    { "", "OVE", "", "GROV" },
    { "", "OW", "L", "GRw" },
    { "", "OW", "", "GRO" },
    { "", "O", "SS", "GRO" },
    { "", "", "", "GR" },
};
static const fw_rule_list l_gr = { r_gr, 18 };

/* lts_rules_g at $845E: GU */
static const fw_rule r_gu[] = {
    { "", "", "ARD", "G" },
    { "", "ATEMA", "LA", "GWaTxMh" },
    { "", "ERN", "SEY", "GrN" },
    { "", "", "ETTE", "G" },
    { " ", "IANA", "", "GbhNx" },
    { " ", "IAN", "ESE", "GbxN" },
    { "", "IDE", "", "GID" },
    { "", "I", "LD", "Gi" },
    { "", "INEA", "", "GiNb" },
    { "", "INN", "", "GiN" },
    { "", "ISH", "", "GWis" },
    { "", "ITAR", "", "GiTh|" },
    { "", "I", "!#", "GI" },
    { "", "", "OU", "GYU" },
    { " ", "", "#", "G" },
    { "!#:", "", "%", "G" },
    { "!#:", "", "#", "GW" },
};
static const fw_rule_list l_gu = { r_gu, 17 };

/* lts_rules_g at $84F7: GW */
static const fw_rule r_gw[] = {
    { "", "", "", "GW" },
};
static const fw_rule_list l_gw = { r_gw, 1 };

/* lts_rules_g at $84FC: GY */
static const fw_rule r_gy[] = {
    { " ", "M", "", "jiM" },
    { " ", "N", "", "GIN" },
};
static const fw_rule_list l_gy = { r_gy, 2 };

static const fw_rule_list *const k_lts_rules_g[] = {
    &l_ga, &l_ge, &l_gg, &l_gh, &l_gi, &l_gl,
    &l_gm, &l_gn, &l_go, &l_gr, &l_gu, &l_gw,
    &l_gy,
};
const fw_rule_table lts_rules_g = { "lts_rules_g", 0x81FD, 782, false, k_lts_rules_g, 13 };

const fw_rule_list *const lts_pairs_g[26] = {
    ['A' - 'A'] = &l_ga,
    ['E' - 'A'] = &l_ge,
    ['G' - 'A'] = &l_gg,
    ['H' - 'A'] = &l_gh,
    ['I' - 'A'] = &l_gi,
    ['L' - 'A'] = &l_gl,
    ['M' - 'A'] = &l_gm,
    ['N' - 'A'] = &l_gn,
    ['O' - 'A'] = &l_go,
    ['R' - 'A'] = &l_gr,
    ['U' - 'A'] = &l_gu,
    ['W' - 'A'] = &l_gw,
    ['Y' - 'A'] = &l_gy,
};

/* lts_rules_h at $850B: HA */
static const fw_rule r_ha[] = {
    { "", "", "BIT", "Ha" },
    { " ", "", "D ", "Ha" },
    { "", "IR", "#", "Hl|" },
    { " ", "LED", " ", "HaLeD" },
    { "", "LL", "#", "HaL" },
    { "", "", "LL", "Ho" },
    { "", "L", "T", "Ho1" },
    { "", "LF", "", "HhF" },
    { "", "LV", "", "HhV" },
    { "", "M", "SHI", "xM" },
    { "#:", "M", " ", "xM" },
    { "", "P", "HAZ", "HaP" },
    { "", "", "ST+", "HA" },
    { "", "RBOUR", "", "HhBx|" },
    { "", "RE", "", "Hl|" },
    { "", "R", "", "Hh|" },
    { "", "", "!!", "Ha" },
    { " ", "S", " ", "HaZ" },
    { " ", "VE", " ", "HaV" },
    { " ", "VEN'T", " ", "HaVxNf" },
    { " ", "VEN", " ", "HAVxN" },
    { " ", "V", "+", "HaV" },
    { "", "W", "K", "Ho" },
};
static const fw_rule_list l_ha = { r_ha, 23 };

/* lts_rules_h at $85CA: HE */
static const fw_rule r_he[] = {
    { " ", "", " ", "~HE" },
    { " ", "", "'", "~HE" },
    { "", "ALTH", "", "He1t" },
    { "", "AR", "D", "Hr" },
    { "", "AR", "T", "Hh" },
    { "", "AR", "K", "Hh" },
    { "", "AV", "Y", "HeV" },
    { "", "AVI", "LY", "HeVi" },
    { "", "IR", "", "l|" },
    { "", "IT", ": ", "HIT" },
    { " ", "LPHOT", " ", "He1PHQT" },
    { " ", "MI", "", "HeMi" },
    { " ", "NCE", " ", "~HeNS" },
    { " ", "R", " ", "~Hr|" },
    { " ", "REFOR", "D", "HeRiFx" },
    { "", "R", "ENCE", "HJR" },
    { "", "RE", "DI", "HxRe" },
    { "", "RE", "TI", "HeRe" },
    { "", "RE", "", "~HJ|" },
    { "", "RO", "IN", "HeRO" },
    { "", "RO", "", "HJRO" },
    { "", "RZEGOVINA", "", "HrTSeGQVbNx" },
    { " ", "RS", " ", "~HrZ" },
    { " ", "RE", "|", "HeRe" },
    { "", "TERO", "", "HeT3O" },
};
static const fw_rule_list l_he = { r_he, 25 };

/* lts_rules_h at $86C4: HI */
static const fw_rule r_hi[] = {
    { " ", "M", "", "~HiM" },
    { "", "NDER", "", "HiNDr|" },
    { " ", "S", " ", "~HiZ" },
    { "", "VE", "", "HIV" },
    { " ", "", " ", "HI" },
};
static const fw_rule_list l_hi = { r_hi, 5 };

/* lts_rules_h at $86EF: HM */
static const fw_rule r_hm[] = {
    { " ", "I", " ", "AceMI" },
};
static const fw_rule_list l_hm = { r_hm, 1 };

/* lts_rules_h at $86FA: HO */
static const fw_rule r_ho[] = {
    { "", "", "LY", "HO" },
    { "", "L", "LAND", "HQ" },
    { "", "ME", "", "HOM" },
    { "", "NDUR", "A:", "HQNjUR" },
    { "", "NE", "ST", "QNi" },
    { "", "N", "EY", "HuN" },
    { "", "NOUR", "", "QNx|" },
    { "", "N", "OR", "QN" },
    { "#:", "O", "D", "Hv" },
    { "", "", "P+", "HO" },
    { "", "RIZONT", "", "HQRiZQNT" },
    { "", "RR", "", "HQR" },
    { "", "RSE", "", "HoS" },
    { "AB", "R", "", "Ho|" },
    { "", "STI", "LE", "HQSfI" },
    { "", "ST", "%", "HOSf" },
    { "", "ST", " ", "HOSf" },
    { " ", "T", "!", "HQT" },
    { " ", "UR", "", "wx|" },
    { "", "US", "ES", "HwZ" },
    { "", "USE", "", "HwS" },
    { " ", "U", "STON", "HU" },
    { " ", "WEV", "ER", "HweV" },
    { "", "V", "ER", "HQV" },
    { " ", "W", " ", "`Hw" },
    { "", "W", " ", "Hw" },
};
static const fw_rule_list l_ho = { r_ho, 26 };

/* lts_rules_h at $87E1: HU */
static const fw_rule r_hu[] = {
    { "", "MOUR", "", "HYUMx|" },
    { "", "NG", "ER", "HunG" },
    { "", "NG", "RY", "HunG" },
    { "", "NDRE", "D", "HuND3x" },
};
static const fw_rule_list l_hu = { r_hu, 4 };

/* lts_rules_h at $880F: HY */
static const fw_rule r_hy[] = {
    { "", "DR", "OGEN", "HID3" },
    { "", "MN", "", "HiM" },
    { "", "P", "ER", "HIP" },
    { "", "PH", "EN", "HIF" },
    { "", "POCRA", "", "HiPQKRx" },
    { "", "POCRI", "", "HiPxKRi" },
    { "", "POTH", "", "HIPQt" },
    { "", "PO", "", "HIPO" },
    { "", "", "ST", "Hi" },
};
static const fw_rule_list l_hy = { r_hy, 9 };

static const fw_rule_list *const k_lts_rules_h[] = {
    &l_ha, &l_he, &l_hi, &l_hm, &l_ho, &l_hu,
    &l_hy,
};
const fw_rule_table lts_rules_h = { "lts_rules_h", 0x850B, 862, false, k_lts_rules_h, 7 };

const fw_rule_list *const lts_pairs_h[26] = {
    ['A' - 'A'] = &l_ha,
    ['E' - 'A'] = &l_he,
    ['I' - 'A'] = &l_hi,
    ['M' - 'A'] = &l_hm,
    ['O' - 'A'] = &l_ho,
    ['U' - 'A'] = &l_hu,
    ['Y' - 'A'] = &l_hy,
};

/* lts_rules_i at $8869: IA */
static const fw_rule r_ia[] = {
    { "", "B", "LE", "IxB" },
    { "", "", "C: ", "ba" },
    { "", "G", "%", "bj" },
    { "#:", "H", " ", "Ix" },
    { "LL", "NC", "+", "YxNS" },
    { "", "NC", "+", "IxNS" },
    { "", "N", "T", "YxN" },
    { "", "", "TE", "bA" },
    { "", "", "TION", "bA" },
    { "", "", "TRIS", "Ix" },
    { "", "", "TURE", "x" },
    { "", "", "!ITY", "ba" },
    { "S", "", ": ", "x" },
    { "G", "", ": ", "x" },
    { "", "", ": ", "J" },
};
static const fw_rule_list l_ia = { r_ia, 15 };

/* lts_rules_i at $88D8: IB */
static const fw_rule r_ib[] = {
    { "!!", "E", "", "IB" },
    { "", "", "ILIT", "iB" },
    { "IG", "", "LE", "xB" },
    { "", "", "LE", "iB" },
    { "", "U", "T", "iBYU" },
    { " ", "M", " ", "IBEeM" },
};
static const fw_rule_list l_ib = { r_ib, 6 };

/* lts_rules_i at $8908: IC */
static const fw_rule r_ic[] = {
    { "", "AL", "LY", "ik" },
    { "", "", "AT+", "iK" },
    { "", "", "A! ", "ik" },
    { "", "IEN", "", "isxN" },
    { "", "", "IES ", "iS" },
    { "", "", "IOUS", "is" },
    { "", "I", "#", "is" },
    { "", "S", " ", "ikS" },
    { "", "", "UL#", "iK" },
    { " :", "", "Y", "IS" },
    { " :", "", "%", "IS" },
    { "", "", " ", "ik" },
    { "", "", "+", "iS" },
};
static const fw_rule_list l_ic = { r_ic, 13 };

/* lts_rules_i at $8964: ID */
static const fw_rule r_id[] = {
    { " ", "EN", "T", "IDeN" },
    { "#:", "EN", "CE: ", "iDxN" },
    { "#:", "EN", "T: ", "iDxN" },
    { "#:", "EN", "C#", "iDeN" },
    { "#:", "EN", "T#", "iDeN" },
    { " ", "IO", "!: ", "iDJ" },
    { ":", "", "L+", "ID" },
    { "", "", "% ", "ID" },
    { "S", "E", "", "ID" },
    { " ", "EA", "", "IDJ" },
};
static const fw_rule_list l_id = { r_id, 10 };

/* lts_rules_i at $89C8: IE */
static const fw_rule r_ie[] = {
    { " :!", "", "D", "I" },
    { "", "D", " ", "bD" },
    { "", "N", "CE", "JN" },
    { "", "N", "T", "JN" },
    { " :!", "R", "C+", "J" },
    { " :!", "R", "", "Ix|" },
    { "", "R", "", "J|" },
    { "", "", "ST", "Yi" },
    { " :!", "", "S ", "I" },
    { "", "S", " ", "bZ" },
    { "", "T", "+", "IxT" },
    { " :!", "", " ", "I" },
    { "", "", "", "E" },
};
static const fw_rule_list l_ie = { r_ie, 13 };

/* lts_rules_i at $8A1F: IF */
static const fw_rule r_if[] = {
    { " ", "", " ", "~iF" },
    { "#!", "IE", ": ", "iFI" },
    { "#!", "Y", " ", "iFI" },
    { "#!", "Y", "ING", "iFI" },
    { "", "", "L", "IF" },
    { "", "U", "L", "iFx" },
};
static const fw_rule_list l_if = { r_if, 6 };

/* lts_rules_i at $8A52: IG */
static const fw_rule r_ig[] = {
    { "", "HT", "", "IT" },
    { "", "H", "", "I" },
    { "", "", "IES", "ij" },
    { "", "", "ING", "Ij" },
    { "", "N", ": ", "IN" },
    { "", "N", "!", "IN" },
    { "", "N", "ABLE", "IN" },
    { "", "N", "AT+", "iGN" },
    { "", "N", "%", "IN" },
    { "", "U", "OUS", "iGYU" },
    { "", "U", "ITY", "iGYU" },
};
static const fw_rule_list l_ig = { r_ig, 11 };

/* lts_rules_i at $8AA4: IH */
static const fw_rule r_ih[] = {
    { "", "OO", "D", "iHv" },
};
static const fw_rule_list l_ih = { r_ih, 1 };

/* lts_rules_i at $8AAD: II */
static const fw_rule r_ii[] = {
    { "APOLLO ", "", " ", "TU" },
    { "EUROPA ", "", " ", "TU" },
    { "SERIES ", "", " ", "TU" },
};
static const fw_rule_list l_ii = { r_ii, 3 };

/* lts_rules_i at $8AD2: IK */
static const fw_rule r_ik[] = {
    { "", "", "E", "Ik" },
};
static const fw_rule_list l_ik = { r_ik, 1 };

/* lts_rules_i at $8AD8: IL */
static const fw_rule r_il[] = {
    { "", "", "E: ", "IL" },
    { "", "", "ITY ", "iL" },
    { "", "", "ITIES ", "iL" },
    { "", "", "D", "I1" },
};
static const fw_rule_list l_il = { r_il, 4 };

/* lts_rules_i at $8AF7: IM */
static const fw_rule r_im[] = {
    { "", "I", "L", "iMi" },
    { "", "I", "T", "iMi" },
    { "", "I", "D", "iMi" },
    { "", "UM", "", "iMxM" },
    { "", "ON", "Y", "iMxN" },
    { "", "ON", "IOUS", "iMON" },
};
static const fw_rule_list l_im = { r_im, 6 };

/* lts_rules_i at $8B2A: IN */
static const fw_rule r_in[] = {
    { ":#:", "A", " ", "ENx" },
    { ":#:", "AS", " ", "ENxZ" },
    { "", "CH", "", "iNc" },
    { "", "COM", "#", "inKxM" },
    { " ", "DEN", "", "iNDeN" },
    { "", "D", "EX", "iND" },
    { "", "", "DIA", "iN" },
    { " ", "DICT", "", "iNDIT" },
    { "", "DIVI", "", "iNDiVi" },
    { "", "DONES", "IA", "iNDONEZ" },
    { "!", "", "D", "IN" },
    { ":#:", "ESS", "", "iNxS" },
    { ":#:", "", "ENT ", "iN" },
    { ":#:", "", "ENCY ", "iN" },
    { ":!#:!", "ES", " ", "ENZ" },
    { ":!#:!", "E", "'S ", "EN" },
    { ":!#:!", "E", " ", "EN" },
    { "#:!", "", "E", "IN" },
    { "", "FER", "IO", "iNFJR" },
    { "", "FORM", "AT", "iNFxM" },
    { "", "FINI", "TE", "iNFxNx" },
    { "", "FIN", "", "iNFiN" },
    { "", "FRARE", "D", "iNFRxRe" },
    { "", "FRA", "", "iNFRx" },
    { "B", "G", "+", "iNj" },
    { "T", "G", "+", "iNj" },
    { "CR", "G", "+", "iNj" },
    { "FR", "G", "+", "iNj" },
    { "H", "G", "+", "iNj" },
    { "IMP", "G", "+", "iNj" },
    { "TW", "G", "+", "iNj" },
    { "", "G", "EN", "iNj" },
    { ":#:!", "", "OUS", "iN" },
    { ":#:!", "O", " ", "ENO" },
    { "", "STAN", "!", "iNSTxN" },
    { "", "TERE", "ST", "iNT3x" },
    { "", "TER", "FAC+", "iNTr" },
    { "", "TERFER", "E", "iNTxFJ|" },
    { "", "TER", "IOR", "iNTJR" },
    { "", "TERN", "AL", "iNTrN" },
    { "", "TERNA", "TION", "iNTxNa" },
    { "", "TERPRE", "T", "iNTrP3x" },
    { "", "TERR", "%", "iNTr" },
    { "", "TERR", "OGA", "iNTeR" },
    { "", "TER", ":#:#TION: ", "iNTr" },
    { "", "TER", ":#: ", "iNTr" },
    { "", "TER", "!", "iNTx" },
    { " ", "TO", " ", "~iNTU" },
    { " ", "TRO", "D", "iNT3x" },
    { "", "VA", "LIDITY", "iNVx" },
    { "", "VA", "LID", "iNVa" },
    { "", "VI", "TE", "iNVI" },
    { " ", "", " ", "~iN" },
    { " :", "US", "", "INxS" },
    { " ", "", "", "iN" },
    { "U", "", "", "iN" },
    { "L", "", "+", "IN" },
    { "", "", "U", "iN" },
};
static const fw_rule_list l_in = { r_in, 58 };

/* lts_rules_i at $8DA5: IO */
static const fw_rule r_io[] = {
    { " ", "N", "", "IxN" },
    { "", "UR", "", "Yx|" },
    { "C", "US", "", "xS" },
    { "T", "US", "", "xS" },
    { "", "US", "", "JS" },
    { "T", "N", "", "xN" },
    { "&", "N", "", "xN" },
    { "", "N", ": ", "JN" },
    { "#R", "R", " ", "J|" },
    { "#R", "R", "ITY ", "bQR" },
    { " ", "U", " ", "IOYU" },
};
static const fw_rule_list l_io = { r_io, 11 };

/* lts_rules_i at $8DF8: IQ */
static const fw_rule r_iq[] = {
    { "", "UIT", "", "iKWiT" },
    { "", "U", "%", "EK" },
};
static const fw_rule_list l_iq = { r_iq, 2 };

/* lts_rules_i at $8E09: IR */
static const fw_rule r_ir[] = {
    { " ", "", "ANIAN", "iR" },
    { " ", "A", "!I ", "iRa" },
    { " ", "A", "! ", "iRa" },
    { "", "E", "!!:", "IRe" },
    { "#:", "", "%", "Ix|" },
    { "", "", "+", "Ix|" },
    { "", "ON", "+", "IRxN" },
    { "", "ON", "", "IxN" },
    { " ", "R", "", "iR" },
    { "#:", "", " ", "x|" },
    { " ", "", "#", "IR" },
    { "", "", "", "r|" },
};
static const fw_rule_list l_ir = { r_ir, 12 };

/* lts_rules_i at $8E65: IS */
static const fw_rule r_is[] = {
    { " ", "", " ", "~iZ" },
    { "PERT", "E", "", "EZ" },
    { "", "H", "", "is" },
    { "", "", "ITION", "iZ" },
    { "", "LAM", "", "iZLaM" },
    { " ", "", "L", "I" },
    { "", "M", " ", "iZm" },
    { "", "ON", "", "iZxN" },
    { "", "RAELITE", "", "iZRxLIT" },
    { "", "RAE", "L", "iZRA" },
    { "", "TAN", "I ", "iSfhN" },
    { "", "TAN", " ", "iSfaN" },
    { "#!!", "", " ", "iS" },
    { "", "", "%", "IZ" },
};
static const fw_rule_list l_is = { r_is, 14 };

/* lts_rules_i at $8EE0: IT */
static const fw_rule r_it[] = {
    { " ", "A", "LI", "iTa" },
    { " ", "A", "LY", "iTx" },
    { " ", "EM", "", "ITxM" },
    { " ", "ER", "A", "iTxR" },
    { "", "", "IC", "iT" },
    { "", "", "IES", "iT" },
    { "", "IN", "ERARY", "ITiN" },
    { "", "IS", " ", "ITiS" },
    { "#:!", "", "OR", "iT" },
    { "", "T", "LE", "iT" },
    { "", "UDE", "", "iTYUD" },
    { "", "", "ULAR", "iT" },
    { "", "", "URE", "ic" },
    { "", "", "Y", "iT" },
};
static const fw_rule_list l_it = { r_it, 14 };

/* lts_rules_i at $8F57: IU */
static const fw_rule r_iu[] = {
    { "", "M", " ", "bxM" },
};
static const fw_rule_list l_iu = { r_iu, 1 };

/* lts_rules_i at $8F5F: IV */
static const fw_rule r_iv[] = {
    { "#:", "", "E", "iV" },
    { "", "I", "T", "iVx" },
    { "", "", "OLOUS", "iV" },
    { "", "", "+", "IV" },
};
static const fw_rule_list l_iv = { r_iv, 4 };

/* lts_rules_i at $8F7C: IZ */
static const fw_rule r_iz[] = {
    { "", "", "ATION", "IZ" },
    { "", "", "ABLE", "IZ" },
    { "", "", "%", "IZ" },
};
static const fw_rule_list l_iz = { r_iz, 3 };

static const fw_rule_list *const k_lts_rules_i[] = {
    &l_ia, &l_ib, &l_ic, &l_id, &l_ie, &l_if,
    &l_ig, &l_ih, &l_ii, &l_ik, &l_il, &l_im,
    &l_in, &l_io, &l_iq, &l_ir, &l_is, &l_it,
    &l_iu, &l_iv, &l_iz,
};
const fw_rule_table lts_rules_i = { "lts_rules_i", 0x8869, 1834, false, k_lts_rules_i, 21 };

const fw_rule_list *const lts_pairs_i[26] = {
    ['A' - 'A'] = &l_ia,
    ['B' - 'A'] = &l_ib,
    ['C' - 'A'] = &l_ic,
    ['D' - 'A'] = &l_id,
    ['E' - 'A'] = &l_ie,
    ['F' - 'A'] = &l_if,
    ['G' - 'A'] = &l_ig,
    ['H' - 'A'] = &l_ih,
    ['I' - 'A'] = &l_ii,
    ['K' - 'A'] = &l_ik,
    ['L' - 'A'] = &l_il,
    ['M' - 'A'] = &l_im,
    ['N' - 'A'] = &l_in,
    ['O' - 'A'] = &l_io,
    ['Q' - 'A'] = &l_iq,
    ['R' - 'A'] = &l_ir,
    ['S' - 'A'] = &l_is,
    ['T' - 'A'] = &l_it,
    ['U' - 'A'] = &l_iu,
    ['V' - 'A'] = &l_iv,
    ['Z' - 'A'] = &l_iz,
};

/* lts_rules_j at $8F93: JA */
static const fw_rule r_ja[] = {
    { "", "MAI", "CA", "jxMA" },
    { "", "NUA", "RY", "jaNYUx" },
    { "", "PAN", "ESE", "jaPxN" },
    { "", "PAN", "", "jxPaN" },
};
static const fw_rule_list l_ja = { r_ja, 4 };

/* lts_rules_j at $8FC3: JE */
static const fw_rule r_je[] = {
    { "", "R", "SEY", "jr" },
    { " ", "SUS", " ", "jEZxS" },
};
static const fw_rule_list l_je = { r_je, 2 };

/* lts_rules_j at $8FD8: JO */
static const fw_rule r_jo[] = {
    { "", "UR", "", "jr" },
};
static const fw_rule_list l_jo = { r_jo, 1 };

/* lts_rules_j at $8FDF: JU */
static const fw_rule r_ju[] = {
    { " ", "LIE", "T", "jULbe" },
    { "", "LY", "", "jULI" },
};
static const fw_rule_list l_ju = { r_ju, 2 };

static const fw_rule_list *const k_lts_rules_j[] = {
    &l_ja, &l_je, &l_jo, &l_ju,
};
const fw_rule_table lts_rules_j = { "lts_rules_j", 0x8F93, 97, false, k_lts_rules_j, 4 };

const fw_rule_list *const lts_pairs_j[26] = {
    ['A' - 'A'] = &l_ja,
    ['E' - 'A'] = &l_je,
    ['O' - 'A'] = &l_jo,
    ['U' - 'A'] = &l_ju,
};
