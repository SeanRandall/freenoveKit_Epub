/* The paragraph driver (patent 140, REFERENCE 3.2b, 3.11): TextExpander_Paragraph cleans up the paragraph text,
   cuts it into words (Tokenize_SplitSeparators, Tokenize_FirstPass, ClassifyWord), joins spelled letters, then
   expands each word through the number, abbreviation and acronym stages and appends the result to the output. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void rep(char *s, const char *find, const char *repl) { fe_str_replace_all(s, find, repl, s); }

/* strncat with an int count: MSVC takes a negative count as a huge one and copies the whole string */
static void ncat(char *d, const char *s, int n) { strncat(d, s, (size_t)n); }

/* The output: the text is cut before the last blank in its first 79 characters (at 78 characters when there is none)
   into pieces that are copied to out_buf one after another, each followed by a blank. The rest gets "\n" unless it
   has one, and every "\n" is written as "\r\n". Nothing is written when the text might not fit (with room for one
   CR per 25 characters). The text is cleared. Also used by TextReaderControl for the lines it emits unexpanded.
   @0x1002e180 */
void fe_Output_AppendText(TextProc *t)
{
    char line[80 + 8], tmp[2000 + 8];
    char *text = t->text, *p, *end, *q;
    size_t n = strlen(text);
    int full = 0;

    memset(tmp, 0, sizeof tmp);
    end = t->out_buf + t->out_size;
    p = t->out_buf + strlen(t->out_buf);
    if (strlen(t->out_buf) + n / 25 + strlen(text) > t->out_size) full = 1;
    if (full) goto done;
    while (strlen(text) > 78) {
        short k = 79, skip;
        while (text[k - 1] != ' ') {
            k--;
            if (k < 1) break;
        }
        skip = k;
        if (k < 2) {
            /* No blank: the original breaks after 77 characters and goes on after 78, so the 78th is lost. */
            k = 78;
            skip = fe_fixes ? 77 : 78;
        }
        line[0] = 0;
        ncat(line, text, k - 1);
        strcat(line, " ");
        if (line[0])
            for (q = line; p < end - 1; ) {
                *p++ = *q++;
                if (!*q) break;
            }
        strcpy(tmp, text + skip);
        strcpy(text, tmp);
    }
    if (strlen(text) > 1 && !strchr(text, '\n')) {
        strcpy(tmp, text);
        strcat(tmp, "\n");
    } else {
        strcpy(tmp, text);
    }
    if (tmp[0])
        for (q = tmp; p < end - 2; ) {
            char c = *q;
            if (c == '\n') *p++ = '\r';
            q++;
            *p++ = c;
            if (!*q) break;
        }
    memset(text, 0, 2000);
done:
    *p = 0;
}

/* Spelled letters: a run of three or more one-letter words (no separator after them; punctuation only on the last,
   or when the next word is longer than one letter) becomes one word "~" + the letters + the last one's punctuation,
   in the last one's slot. The others and all their QUALs become "". "I B M" -> "", "", "~IBM". @0x10043fb0 */
static void join_flush(TextProc *t, short i, const char *acc)
{
    short len = (short)strlen(acc);
    for (short j = i - len; j <= i - 2; j++) {
        strcpy(t->word[j], "");
        strcpy(t->qual[j], "");
    }
    char *w = t->word[i - 1];
    if (fe_fixes) {
        /* the word buffers hold 60 characters (TextExpander_Paragraph) */
        char *nw = fe_malloc(strlen(acc) + strlen(t->punc[i - 1]) + 64);
        fe_free(w);
        t->word[i - 1] = w = nw;
    }
    strcpy(w, "~");
    strcat(w, acc);
    strcat(w, t->punc[i - 1]);
    strcpy(t->qual[i - 1], "");
}

void fe_Tokenize_JoinLetters(TextProc *t)
{
    char acc[FE_MAXWORDS + 64], w[2048];

    acc[0] = 0;
    for (short i = 1; t->cur_word + 1 >= i; i++) {
        w[0] = 0;
        if (t->cur_word >= i) ncat(w, t->word[i], (int)strlen(t->word[i]) - (int)strlen(t->punc[i]));
        else strcat(w, "@");
        if ((short)strlen(w) == 1 && ((w[0] >= 'a' && w[0] <= 'z') || (w[0] >= 'A' && w[0] <= 'Z')) &&
            (strlen(t->punc[i]) == 0 || t->cur_word == i ||
             (unsigned)(strlen(t->word[i + 1]) - strlen(t->punc[i + 1])) > 1) &&
            strlen(t->sepr[i]) == 0) {
            /* The original collects the letters in a 60-byte stack buffer: 60 letters overwrite its return address
               (and the word buffer holds 58). */
            if (fe_fixes && strlen(acc) >= 55) {
                join_flush(t, i, acc);
                acc[0] = 0;
            }
            strcat(acc, w);
            continue;
        }
        if ((short)strlen(acc) >= 3) join_flush(t, i, acc);
        acc[0] = 0;
    }
}

/* Patent Table 2 (0x100bd0a8) and more frequent words (0x100bcff4): never abbreviations */
static const char freq1[] = "#the#of#and#you#that#was#are#with#his#they#this#have#by#one#had#not#but#what#were#when"
                            "#there#your#which#their#";
static const char freq2[] = "#will#each#about#out#them#then#she#many#some#these#would#other#into#more#two#like#time"
                            "#could#make#than#been#now#did#down#only#find#water#very#after#words#call#where#most#know#has#";
/* all-capital abbreviations expanded on their own (0x100bcfd8) */
static const char units[] = "#c#f#k#fyi#kb#mb#gb#kw#kwh#";
/* "a/b" pairs read "a or b" (0x100bcf48) */
static const char pairs[] = "#he/she#him/her#his/hers#male/female#boy/girl#over/under#odd/even#forward/reverse"
                            "#before/after#hot/cold#singular/plural#up/down#old/new#in/out#";

/* Clean-up of the paragraph text before it is cut into words. */
static void clean_text(char *text, char *buf)
{
    static const char *const smileys[][2] = {
        {"*<:-)", " ho ho ho "}, {":-)", " ha ha "}, {"8-)", " ha ha "}, {";-)", " wink "}, {":-(", " groan "},
        {":-o", " uh oh "}, {":)", " ha ha "}, {";)", " wink "}, {":o", " uh oh "}, {":-/", " oh yeah? "},
        {">:->", " heh heh "}, {">;->", " heh heh "}, {"|-O", " yawn "}, {"<-->", " to and from "},
        {"<->", " to and from "}, {"-->", " to "}, {"->", " to "}, {"<==>", " to and from "}, {"<=>", " to and from "},
        {"==>", " to "}, {"=>", " to "}, {"  ", " "}};
    /* runs of four drop every pair ("____" -> "", "_____" -> "_"); two backslashes drop every backslash */
    static const char *const runs[][2] = {
        {"____", "__"}, {"----", "--"}, {"====", "=="}, {"-=-=", "-="}, {"-_-_", "-_"}, {"+-+-", "+-"},
        {"++++", "++"}, {"+_+_", "+_"}, {"****", "**"}, {">>>>", ">>"}, {"<<<<", "<<"}, {"<><>", "<>"},
        {"%%%%", "%%"}, {"@@@@", "@@"}, {"####", "##"}, {"^^^^", "^^"}, {"||||", "||"}, {"////", "//"},
        {"\\\\", "\\"}, {"&&&&", "&&"}, {"::::", "::"}, {";;;;", ";;"}};
    static const char *const marks[][2] = {
        {"??", "?"}, {"!!", "!"}, {"......", ".. "}, {".....", ".. "}, {"....", ".. "}, {"...", ".. "}};
    static const char *const words[][2] = {
        {"$$$", "$$"}, {" $$ ", " dollars "}, {" $s ", " dollars "}, {" $'s ", " dollars "},
        {" !s ", ", exclamations!, "}, {" !'s ", ", exclamations!, "}, {"(!)", " (exclamation!) "},
        {"\"!\"", " \"exclamation!\" "}, {" ?s ", ", questions?, "}, {" ?'s ", ", questions?, "},
        {"(?)", " (question?) "}, {"\"?\"", " \"question?\" "}, {" : ", " colon "}, {" ; ", " semicolon "},
        {" ~", " approximately "}};
    static const char *const parens[][2] = {
        {"( ", "("}, {" )", ")"}, {"(", "{{"}, {"{{", " ("}, {")", "}}"}, {"}}", ") "}, {") .", ")."},
        {") ,", "),"}, {") .", ")."}, {") ?", ")?"}, {"  ", " "}, {" (s) ", "(s) "}};
    static const char *const bars[][2] = {
        {"| -", "|"}, {" |  |", ". "}, {" | |", ". "}, {"| ", ", "}, {"--", " $%& "}, {" $%& ", " -- "}, {"  ", " "}};
    short pos, n;
    size_t k;

    /* Line breaks: a line shorter than 39 characters ends in "..", a longer one in a blank */
    pos = 0;
    buf[0] = 0;
    while ((n = fe_strpos(text + pos, "\n")) != 0) {
        ncat(buf, text + pos, n - 1);
        strcat(buf, n - 1 < 0x27 ? ".." : " ");
        pos += n;
    }
    strcat(buf, text + pos);
    strcpy(text, buf);
    if ((short)strlen(text) <= 0x794)
        for (k = 0; k < sizeof smileys / sizeof smileys[0]; k++) rep(text, smileys[k][0], smileys[k][1]);
    for (k = 0; k < sizeof runs / sizeof runs[0]; k++)
        if (fe_strpos(text, runs[k][0])) rep(text, runs[k][1], "");
    for (k = 0; k < sizeof marks / sizeof marks[0]; k++) rep(text, marks[k][0], marks[k][1]);
    if ((short)strlen(text) <= 0x794)
        for (k = 0; k < sizeof words / sizeof words[0]; k++) rep(text, words[k][0], words[k][1]);
    if (fe_strpos(text, "("))
        for (k = 0; k < sizeof parens / sizeof parens[0]; k++) rep(text, parens[k][0], parens[k][1]);
    /* a blank after the first "..." */
    if ((n = fe_strpos(text, "...")) != 0) {
        buf[0] = 0;
        ncat(buf, text, n + 2);
        strcat(buf, " ");
        strcat(buf, text + n + 2);
        strcpy(text, buf);
    }
    for (k = 0; k < sizeof bars / sizeof bars[0]; k++) rep(text, bars[k][0], bars[k][1]);
}

/* A separator is written after the word only around quotes and brackets: the word has PREPUNC, or is at most two
   characters with a quote or "[", and the next word is at most three characters with a closing quote or bracket. */
static int keep_separator(TextProc *t, short i)
{
    if ((short)strlen(t->prepunc[i]) <= 0) {
        if ((short)strlen(t->out_text) > 2) return 0;
        if (!fe_strpos(t->out_text, "\"") && !fe_strpos(t->out_text, "'") && !fe_strpos(t->out_text, "[")) return 0;
    }
    const char *nx = t->word[i + 1];
    if ((short)strlen(nx) > 3) return 0;
    return fe_strpos(nx, "\"") || fe_strpos(nx, "'") || fe_strpos(nx, "]") || fe_strpos(nx, ")");
}

/* A lone "-" before word j is read "minus" when word j is a number, "$..." or "...%" (without its punctuation). */
static int minus_before(TextProc *t, short j)
{
    const char *w = t->word[j];
    int l = (int)strlen(w) - (int)strlen(t->punc[j]);
    if (t->num[j] || !strncmp(w, "$", 1)) return 1;
    /* The original reads in front of a word that is all punctuation. */
    if (fe_fixes && l < 1) return 0;
    return !strncmp(w + l - 1, "%", 1);
}

/* lc_period as Tokenize_Dots sets it: the word ends in '.' (before a final '"'), or its run of closing punctuation
   starts with one */
static int period_at_end(const char *w)
{
    size_t l = strlen(w), e;
    if (l && w[l - 1] == '"') l--;
    if (l && w[l - 1] == '.') return 1;
    for (e = l; e && strchr(".,;:?!)]", w[e - 1]); e--)
        ;
    return e < l && w[e] == '.';
}

/* Returns 0, or -1 when out of memory (the last Tokenize_FirstPass return value otherwise). @0x1002b8d0 */
short fe_TextExpander_Paragraph(TextProc *t)
{
    const unsigned char *fr = fe_frame_of(0x1002b8d0);
    char *text = t->text, *buf, *r;
    char sep[16], w[2048], b2[256], key[256], first[256], lcd[256];
    short ret = 0, pos, end = 0, n, i, feet, quote, res, k, start, last, r12;

    memset(sep, 0, sizeof sep);
    if (fr) {   /* the stack bytes the first Tokenize_SplitSeparators leaves alone when it finds nothing */
        memcpy(&end, fr + 0x1a, 2);
        memcpy(sep, fr + 0x24, 8);
    }
    buf = fe_malloc(0x7d1 + 0x800);    /* 0x7d1 in the original */
    if (!buf) {
        ret = -1;
        goto out;
    }
    r = fe_str_trim_leading(text);
    if (r != text) strcpy(text, r);
    strcat(text, " ");
    rep(text, "\t", " ");
    rep(text, "  ", " ");
    clean_text(text, buf);
    if (strlen(text) <= 1) goto emit;

    /* ---- the words ---- */
    pos = 1;
    t->cur_word = 0;
    t->accepted_text[0] = 0;
    fe_emph_state = 0;
    rep(text, "\n", "");
    rep(text, "\r", "");
    r = fe_str_trim_spaces(text);
    if (r != text) strcpy(text, r);
    strcat(text, " ");
    do {
        short c;
        fe_Tokenize_SplitSeparators(t, pos, &end, sep);
        if (end == 0 || t->cur_word >= FE_MAXWORDS - 1) break;
        c = ++t->cur_word;
        t->sep_dash[c] = 0;
        if (!strcmp(sep, "- ")) {
            t->sep_dash[c] = 1;
            strcpy(sep, "-");
        }
        t->sepr[c] = fe_strdup(sep);
        n = end - pos;
        if (n > 30) n = 30;
        if (n == 0) {
            n = 1;
            strcpy(t->sepr[c], "");
        }
        w[0] = 0;
        ncat(w, text + pos - 1, n);
        t->word[c] = fe_strdup("                                                            ");
        strcpy(t->word[c], w);
        ret = fe_Tokenize_FirstPass(t, pos, &end);
        if (ret == -1) goto out;
        fe_ClassifyWord(t);
        /* after "+" (not after a number) the next word starts at the "+" */
        if (pos != end && sep[0] == '+' && !t->num[t->cur_word]) pos = end;
        else pos = end + 1;
        if ((short)strlen(text) < pos) break;
        if (!strncmp(text + pos - 1, " ", 1)) pos++;
    } while ((short)strlen(text) >= pos);
    fe_Tokenize_JoinLetters(t);

    /* ---- the expansion, word by word ---- */
    buf[0] = 0;
    quote = 0;
    res = 0;
    feet = 0;
    for (i = 1; t->cur_word >= i; i++) {
        /* res (a rule, "minus", a rating or an acronym expanded the word) is cleared only before the abbreviation
           search: a number, time or frequent word after an accepted word inherits it, which skips its slash handling
           and Classify_PeriodContext ("The low was -5" at the end gets no final "."). */
        if (fe_fixes) res = 0;
        if (strlen(t->word[i]) == strlen(t->punc[i])) {
            strcpy(t->word[i], " ");
            strcat(t->word[i], t->punc[i]);
        }
        if (strlen(t->word[i]) <= 0x3a) {
            strcpy(t->word_lc, t->word[i]);
        } else {
            t->word_lc[0] = 0;
            strncat(t->word_lc, t->word[i], 0x3a);
        }
        if (strlen(t->word_lc) == 0) continue;
        /* Only Tokenize_Dots sets lc_period, and frequent words, numbers, times and "minus" skip it: they take the
           previous word's, and Classify_PeriodContext gives them a sentence-end blank before a capital ("Ala. and
           Boise", "Dr. 5 Then"). */
        if (fe_fixes) t->lc_period = (short)period_at_end(t->word_lc);
        strcpy(t->out_text, t->word_lc);
        if (!strcmp(t->prepunc[i], "\"") || !strcmp(t->word_lc, " \"")) quote = 1;
        strcpy(w, "#");
        ncat(w, t->out_text, (int)strlen(t->out_text) - (int)strlen(t->punc[i]));
        fe_strlwr(w);
        strcat(w, "#");
        if (fe_strpos(freq1, w) || fe_strpos(freq2, w)) {
            /* A frequent word is never an abbreviation. In capitals, apart from capitals other than I and A within two
               words, it gets "~". */
            if (t->auc[i]) {
                for (k = 1; k <= 2; k++) {
                    short j = i - k;
                    if (j >= 1 && t->auc[j] && strcmp(t->word[j], "I") && strcmp(t->word[j], "A")) goto after;
                }
                for (k = 1; k <= 2; k++) {
                    short j = i + k;
                    if (t->cur_word >= j && t->auc[j] && strcmp(t->word[j], "I") && strcmp(t->word[j], "A")) goto after;
                }
                strcpy(w, "~");
                strcat(w, t->out_text);
                strcpy(t->out_text, w);
            }
            goto after;
        }
        if (!strcmp(t->qual[i], "roman") || !strcmp(w, "#x#") || !strcmp(w, "#i#") || !strcmp(w, "#v#"))
            if (i == 1 || strlen(t->out_text) != 4 || strlen(t->word[i - 1]) != 5 || t->num[i - 1] != 1 ||
                strcmp(t->sepr[i - 1], "-"))
                fe_Num_Roman(t, i);
        if (!strcmp(t->word[i], "-") && !t->sep_dash[i] && t->cur_word >= i + 1 && minus_before(t, i + 1)) {
            strcpy(t->out_text, "minus");
            res = 1;
            goto after;
        }
        if (t->num[i] && t->cur_word >= i) {
            fe_NumberExpander(t, i, &feet, &quote);
            goto after;
        }
        r12 = 0;
        if (t->cur_word >= i + 1 && (!strcmp(t->sepr[i], "-") || !strcmp(t->word[i + 1], "-"))) {
            fe_Num_YearsRanges(t, i, &r12);
            if (r12) goto after;
        }
        if (!strcmp(t->qual[i], "timenum")) {
            fe_Num_Time(t, i);
            goto after;
        }
        if (fe_strpos(t->out_text, ":")) {
            /* digits with ":" (and "." ",") that are not a time: "1:2" -> "1 to 2" */
            short si;
            for (si = 1; (short)strlen(t->out_text) >= si; si++) {
                char c = t->out_text[si - 1];
                if (!((c >= '0' && c <= '9') || c == ':' || c == '.' || c == ',')) break;
            }
            if ((short)strlen(t->out_text) < si) {
                rep(t->out_text, ":", " to ");
                goto after;
            }
        }
        if (quote) {
            /* word_lc[59] when out_text is empty */
            int l = (int)strlen(t->out_text);
            if (((char *)t->out_text)[l - 1] == '"' && strcmp(t->out_text, " \"")) quote = 0;
        }
        fe_Tokenize_Dots(t);
        if (!strcmp(t->word[i], "A") && i > 1 && (t->iuc[i - 1] || t->auc[i - 1])) {
            strcpy(t->out_text, "aye");
            goto after;
        }
        if (i > 2 && !strcmp(t->word[i - 1], "dot")) goto after;

        /* The abbreviation records whose key is the word (or whose key's first word is, or the word with its dots
           read as blanks). A word in capitals with no capitals within two words is tried only when it has at most
           three letters and the record is a state, direction, city or time, or the word is in the units list. */
        fe_Abbrev_LetterRange(t, &start, &last);
        t->rule_type[0] = 0;
        res = 0;
        for (k = start; k <= last; k++) {
            const FeTriple *rec = &fe_abbrev[k];
            strcpy(key, rec->key);
            strcpy(w, key);
            fe_strlwr(w);
            strcpy(key, w);
            strcpy(first, w);
            if ((n = fe_strpos(key, " ")) != 0) {
                w[0] = 0;
                ncat(w, first, n - 1);
                strcpy(first, w);
            }
            strcpy(lcd, t->word_lc);
            rep(lcd, ".", " ");
            if (strcmp(t->word_lc, first) && strcmp(lcd, key)) continue;
            strcpy(t->rec_keyword, rec->key);
            strcpy(t->rec_expansion, rec->exp);
            strcpy(t->rule_type, rec->rule);
            /* The original tests the word after next as auc[i + 1] again. */
            if (t->auc[i] && !(i - 2 >= 1 && t->auc[i - 2]) && !(i - 1 >= 1 && t->auc[i - 1]) &&
                !(t->cur_word >= i + 1 && t->auc[i + 1]) && !(i + 2 <= t->cur_word && t->auc[fe_fixes ? i + 2 : i + 1])) {
                strcpy(b2, "#");
                strcat(b2, t->word_lc);
                strcat(b2, "#");
                if (strlen(t->word_lc) > 3) continue;
                if (strncmp(t->rule_type, "state", 5) && strncmp(t->rule_type, "direction", 9) &&
                    strcmp(t->rule_type, "city") && strcmp(t->rule_type, "time") && !fe_strpos(units, b2))
                    continue;
            }
            if (t->iuc[i]) {   /* an initial capital carries over to the expansion */
                b2[0] = t->rec_expansion[0];
                b2[1] = 0;
                fe_strupr(b2);
                strcat(b2, t->rec_expansion + 1);
                strcpy(t->rec_expansion, b2);
            }
            fe_AbbrevRuleDispatch(t, i, &res);
            if (res) break;
        }
        if (t->cur_word >= i + 1 && (!strcmp(t->sepr[i], "-") || !strcmp(t->word[i + 1], "-")))
            fe_Num_QualContext(t, i, &res);
        if (!strcmp(t->qual[i], "roman") && atoi(t->out_text)) goto after;
        n = fe_strpos(t->word[i], " ");
        if (res) goto slash;
        if (n) goto after;
        fe_Acronym_MilitaryRating(t, i, &res);
        if (res) goto slash;
        if (strlen(t->word_lc) > 1) fe_AcronymInitialismExpander(t, i, &res);

    after:
        /* "a/b": "he/she" -> "he or she", else "a /" */
        if (res || strcmp(t->sepr[i], "/") || t->auc[i] || t->num[i] || t->cur_word < i + 1 || t->auc[i + 1] ||
            t->num[i + 1])
            goto slash;
        strcpy(w, "#");
        strcat(w, t->out_text);
        strcat(w, "/");
        ncat(w, t->word[i + 1], (int)strlen(t->word[i + 1]) - (int)strlen(t->punc[i + 1]));
        fe_strlwr(w);
        strcat(w, "#");
        if (!strcmp(w, "#fwd/rev#")) {
            strcpy(w, "#forward/reverse#");
            strcpy(t->out_text, "forward");
            strcpy(t->word[i + 1], "reverse");
        }
        if (!strcmp(w, "#sing/pl#")) {
            strcpy(w, "#singular/plural#");
            strcpy(t->out_text, "singular");
            strcpy(t->word[i + 1], "plural");
        }
        strcat(t->out_text, fe_strpos(pairs, w) ? " or" : " /");
        goto per;

    slash:
        /* a word between two slashes ("a/b/c") gets " /" */
        if (!strcmp(t->sepr[i], "/") && (short)strlen(fe_str_trim_spaces(t->out_text)) > 0 &&
            ((i > 1 && !strcmp(t->sepr[i - 1], "/")) || (t->cur_word > i && !strcmp(t->sepr[i + 1], "/"))) &&
            !fe_strpos(t->out_text, "/") && !fe_strpos(t->out_text, "per"))
            strcat(t->out_text, " /");

    per:
        /* "$5/hr": the word after the slash gets "per" */
        if (!res && i > 1 && !strcmp(t->sepr[i - 1], "/") && !strncmp(t->word[i - 1], "$", 1)) {
            strcpy(w, "per ");
            strcat(w, t->out_text);
            strcpy(t->out_text, w);
        }
        if (!strncmp(t->word[i], "$", 1)) fe_Num_Money(t, i);
        if (!res) fe_Classify_PeriodContext(t, i);
        rep(t->out_text, "..", ".");
        if (!strcmp(t->out_text, "")) continue;
        if ((unsigned)(0x7cb - strlen(t->out_text)) < strlen(buf)) break;
        if (fe_strpos(t->prepunc[i], "(")) strcat(buf, "(");
        strcat(buf, fe_str_trim_leading(t->out_text));
        if ((short)strlen(t->sepr[i]) > 0 && t->cur_word >= i + 1 && !fe_strpos(t->out_text, t->sepr[i]) &&
            keep_separator(t, i))
            strcat(buf, t->sepr[i]);
        strcat(buf, " ");
        rep(buf, "   ", "  ");
    }
    for (k = 1; t->cur_word >= k; k++) {
        fe_free(t->word[k]);
        fe_free(t->sepr[k]);
        fe_free(t->qual[k]);
        fe_free(t->punc[k]);
        fe_free(t->prepunc[k]);
    }

    /* ---- the result ---- */
    rep(buf, "-'-s ", "'s ");
    if (!fe_strpos(buf, " o-s ")) {
        rep(buf, "-s ", "'s ");
    } else {   /* "o-s" stays */
        rep(buf, " o-s ", " o-$%& ");
        rep(buf, "-s ", "'s ");
        rep(buf, " o-$%& ", " o-s ");
    }
    rep(buf, "((", "(");
    if (!strncmp(text, ". ..", 4) && !strncmp(buf, ". . ", 4)) rep(buf, ". . ", ". ");
    strcpy(text, fe_str_trim_spaces(buf));
emit:
    fe_Output_AppendText(t);
out:
    fe_free(buf);
    return ret;
}
