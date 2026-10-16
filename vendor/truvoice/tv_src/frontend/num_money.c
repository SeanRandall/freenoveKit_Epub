/* Money: "$5.50" -> "5 dollars and 50 cents", "$1.2 million", "$5M", "$US", singular "5 dollar bill" (REFERENCE 3.8,
   8.4). */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* The original copies these word lists into its 0x1460-byte frame at entry; they sit next to the text buffers, which
   run on into them. Offsets are frame offsets [F+x]. */
static const struct { unsigned short off; const char *s; } money_lists[] = {
    {0x14c, "#million#billion#trillion#thousand#"},
    {0x170, "#more#less#costlier#cheaper#"},
    {0x63c, "#anyone#everyone#he#his#her#him#he's#himself#herself#i#it#its#i'm#i'd#i'll#i've#itself#my#me#much#myself#our#per#pre#re#so#she#such#someone#somebody#this#they#than#them#their#these#those#they'd#there#themselves#us#used#very#we#"},
    {0x720, "#you#your#you'd#you're#yourself#yourselves#be#can#have#the#a#an#am#for#from#of#shall#can't#cannot#couldn't#don't#didn't#isn't#not#never#won't#wasn't#wouldn't#may#must#might#are#been#could#had#has#is#should#was#were#will#would#"},
    {0x328, "#all#any#anything#both#double#each#every#either#everything#few#half#more#most#many#no#none#nothing#neither#some#several#something#and#but#nor#or#"},
    {0x484, "#do#did#does#altho#although#because#even#furthermore#however#henceforth#if#insofar#inasmuch#moreover#since#that#then#though#thereby#therefore#thereupon#unless#while#whether#whereas#whereby#wherein#whenever#"},
    {0x3bc, "#away#about#after#among#along#above#around#across#against#amongst#below#before#behind#beyond#beside#between#during#despite#except#into#inside#over#outside#throughout#upon#under#until#within#without#"},
    {0x230, "#as#at#by#down#in#near#on#out#off#to#toward#through#with#up#wherever#wherupon#wherefore#yet#"},
    {0x190, "#how#who#why#when#what#whom#which#where#whose#who'd#whoever#whatever#"},
    {0x11d0, "#awhile#alternately#busily#changed#closely#cleanly#doubly#early#everywhere#forever#humbly#heavily#hastily#idly#irritably#just#justifiably#meanwhile#momentarily#now#notably#only#perhaps#probably#primarily#politically#rather#"},
    {0x1d8, "#readily#subtly#somehow#separately#too#there#today#thereof#therein#visibly#well#wholly#"},
    {0x1390, "#ask#ally#apply#allow#argue#abuse#affix#adhere#aging#agreed#arrange#asleep#allege#addict#allies#assent#awaken#address#approve#analyze#augment#animate#agonize#analyse#abandon#alienate#activate#advertise#accompany#associate#"},
    {0x804, "#attribute#accustom#ascertain#alternate#affiliate#acquiesce#appropriate#approximate#appropriated#administrate#bow#bled#being#begin#bring#break#bathe#become#bother#borrow#behave#beckon#bottom#bewilder#breathing#come#came#clear#"},
    {0x12b0, "#cover#close#creat#climb#comply#cloth#compel#compete#caring#create#conduct#content#complete#combine#concert#convert#contest#confine#collect#compact#cherish#cleanse#conquer#curtail#console#crochet#consider#contrast#contract#"},
    {0xd5c, "#conflict#compound#coincide#compress#commence#converse#construct#cooperate#conquered#contribute#correspond#coordinate#contradict#characterize#done#deny#defy#dyed#dove#doing#dealt#defer#deign#deter#detail#driven#desert#defect#"},
    {0xe40, "#digest#dilute#diffuse#deafen#dreamt#detest#discard#diffuse#dialyze#default#dispatch#discover#develop#delegate#decrease#dissolve#discount#diminish#diagnose#decipher#deputize#duplicate#discharge#delineate#demarcate#distribute#"},
    {0xf24, "#deliverate#deactivate#desegregate#deteriorate#enter#empty#equip#expel#excuse#escort#export#equate#echoed#exploit#extract#estimate#expedite#evaluate#elongate#endanger#elaborate#emphasize#economize#exaggerate#find#fled#forget#"},
    {0x8e8, "#fallen#forego#fulfill#ferment#frequent#go#get#got#give#gone#geld#going#given#gonna#graduate#held#house#hover#housed#hasten#heighten#homogenize#ignor#imply#infer#impact#insert#ignore#import#injure#indent#invent#insult#impinge#"},
    {0x9cc, "#indulge#invest#ironed#imagine#isolate#incline#invalid#itemize#implant#imprint#incense#increase#initiate#interfere#intervene#intrigue#interpret#implement#improvise#intercept#introduce#incorporate#interchange#jagged#jeopardize#"},
    {0xab0, "#know#knead#let#lie#live#lead#lose#lounge#listen#losing#linger#lunged#loosen#labelled#legistlate#made#meant#misled#maintain#moderate#multiply#motivate#mobilize#memorize#memorise#marbleize#noting#negate#nourish#omit#obey#occur#"},
    {0x554, "#object#oppose#occupy#offset#offend#ordain#outrun#orbit#observe#oppress#occlude#outgrow#objected#outspoken#orchestrate#put#paid#prove#permit#prefer#polish#pursue#preach#proven#pasted#punish#peruse#present#porduce#project#perfect#"},
    {0x1008, "#prevent#proceed#protest#portray#pasting#purport#pollute#preside#pretest#progress#prophesy#permeate#projected#perfected#predicate#prosecute#progressed#prioritize#progresses#perfecting#read#reply#refer#rebel#rune#risen#recite#"},
    {0xb94, "#revere#relay#recap#record#refuse#reject#refund#ranged#resent#reside#relate#refill#repent#rebell#recess#remake#replay#retake#resell#resume#ranging#rebuild#recover#recount#rewrite#reckon#readapt#reelect#rescind#rethink#recheck#"},
    {0x10ec, "#recognize#represent#recommend#recording#reconcile#reiterate#reconsidel#recuperate#rediscover#reestablish#see#say#sit#sat#sew#said#seem#says#sped#swam#skis#sown#start#seize#shove#sever#scour#saute#select#suited#survey#skiing#"},
    {0xc78, "#subject#suggest#segment#suspect#satisfy#solicit#separate#syndicate#stabilize#subsidize#superimpose#subordinate#take#took#told#tell#tear#taste#think#threaten#transport#translate#transplant#use#undo#unite#upset#undergo#uniting#"},
    {0x290, "#vibrate#won#wnet#want#wait#weigh#wind#write#wound#wasted#writhe#withdrew#withheld#withdraw#withdrawn#be#have#do#did#does#are#been#had#has#is#was#were#"},
};
/* the lists that keep "dollars" plural when they hold the word after the amount, in the order tested */
static const unsigned short plural_lists[] = {
    0x63c, 0x720, 0x328, 0x484, 0x3bc, 0x230, 0x190, 0x11d0, 0x1d8, 0x1390, 0x804, 0x12b0,
    0xd5c, 0xe40, 0xf24, 0x8e8, 0x9cc, 0xab0, 0x554, 0x1008, 0xb94, 0x10ec, 0xc78, 0x290,
};

#define FRAME 0x1460
#define FBUF 4096                    /* fe_fixes: size of each text buffer */
static unsigned char money_frame[FRAME + 16 + 5 * FBUF] __attribute__((aligned(16)));

/* The original writes into the word strings in place ("million" -> "million dollars"), past their allocation.
   With fe_fixes a longer string gets a new block. */
static void put_word(char **slot, const char *s)
{
    if (fe_fixes && strlen(s) > strlen(*slot)) {
        fe_free(*slot);
        *slot = fe_strdup(s);
    } else
        strcpy(*slot, s);
}

static void cat_word(char **slot, const char *s)
{
    if (fe_fixes) {
        char *d = fe_malloc(strlen(*slot) + strlen(s) + 1);
        strcpy(d, *slot);
        strcat(d, s);
        fe_free(*slot);
        *slot = d;
    } else
        strcat(*slot, s);
}

/* w + k; the original reads in front of w when k < 0 */
static const char *at(const char *w, int k) { return fe_fixes && k < 0 ? w : w + k; }

/* fe_fixes: strcpy into a 60-byte field, cut to fit (the bytes after the terminator stay as they are) */
static void copy60(char *d, const char *s)
{
    size_t n = strlen(s);
    if (n > 59) n = 59;
    memmove(d, s, n);
    d[n] = 0;
}

/* "#word#" in lower case, without the word's punctuation */
static void hashed(char *d, const TextProc *t, int j)
{
    strncat(d, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(d);
    strcat(d, "#");
}

/* Money, called by the paragraph driver for a word starting with '$' (or "US$"). The amount is rewritten into word_lc
   and at the head of out_text: "$5.50" -> "5 dollars and 50 cents", "$1,000" -> "1,000 dollars", "$5M" / "$5million"
   -> "5 Million dollars"; "$5 million" appends " dollars" to the next word instead. "dollars" becomes "dollar" before
   a word that is not in its function-word / verb lists ("a $5 bill"), and "US" becomes "U.S.". @0x10041800 */
void fe_Num_Money(TextProc *t, short i)
{
    const unsigned char *fr = fe_frame_of(0x10041800);
    unsigned char *F = money_frame;
    char *B1C, *B5C, *B98, *BD4, *B110, *w;
    const char *sfx;
    short bx, bp, si, p18, ax, dx;
    int e, k, n, mag = 0;
    unsigned q;

    /* The five 60-byte text buffers overflow into each other, into the saved index at [F+0x58] and into the word
       lists; without fe_fixes the frame is modelled byte for byte, with fe_fixes each buffer gets FBUF bytes. */
    memset(F, 0, sizeof money_frame);
    if (fr) memcpy(F, fr, FRAME + 16);
    if (fe_fixes) {
        B1C = (char *)F + FRAME + 16; B5C = B1C + FBUF; B98 = B5C + FBUF; BD4 = B98 + FBUF; B110 = BD4 + FBUF;
    } else {
        B1C = (char *)F + 0x1c; B5C = (char *)F + 0x5c; B98 = (char *)F + 0x98; BD4 = (char *)F + 0xd4;
        B110 = (char *)F + 0x110;
    }
    for (q = 0; q < sizeof money_lists / sizeof money_lists[0]; q++)
        memcpy(F + money_lists[q].off, money_lists[q].s, strlen(money_lists[q].s) + 1);
    *(int *)(F + 0x58) = i;
#define I (fe_fixes ? (int)i : *(int *)(F + 0x58))    /* re-read from the frame where the original does */

    /* "$5K" -> "$5,000" in the word itself */
    if (fe_fixes) {
        char tmp[2100];
        fe_str_replace_all(t->word[i], "K", ",000", tmp);
        put_word(&t->word[i], tmp);
    } else
        fe_str_replace_all(t->word[i], "K", ",000", t->word[i]);
    strcpy(B5C, t->word[i]);

    /* A comma after the dot, with 3+ characters after it: "$1.000,500" -> "$1,000" (the comma goes, the dot becomes a
       thousands comma, and everything after 3 digits is cut). "$1.000,50" is too short for the test and stays. */
    p18 = fe_strpos(B5C, ",");
    bp = fe_strpos(B5C, ".");
    if (p18 != 0 && (short)strlen(B5C) - 3 >= p18 && bp != 0 && (short)strlen(B5C) >= bp + 3 && p18 > bp) {
        B1C[0] = 0;
        strncat(B1C, B5C, p18 - 1);
        strcat(B1C, B5C + p18);
        strcpy(B5C, B1C);
        B1C[0] = 0;
        strncat(B1C, B5C, bp - 1);
        if (bp > 2) strcat(B1C, ",");
        strcat(B1C, B5C + bp);
        strcpy(B5C, B1C);
        B1C[0] = 0;
        strncat(B1C, B5C, bp > 2 ? bp + 3 : bp + 2);
        strcat(B1C, t->punc[i]);
        strcpy(B5C, B1C);
        put_word(&t->word[i], B1C);
    }

    e = I + 1;
    if (t->cur_word >= e && !strcmp(t->word[i + 1], "+")) {
        /* "$5 + million": " dollars" goes after "million", else after the "+" */
        B1C[0] = 0;
        strcat(B1C, t->word[i] + 1);
        strcpy(B5C, B1C);
        B1C[0] = 0;
        strcpy(B98, "ZZ!Z");
        if (t->cur_word >= I + 2) {
            B98[0] = 0;
            strncat(B1C, t->word[i + 2], strlen(t->word[i + 2]) - strlen(t->punc[i + 2]));
            strcat(B98, "#");
            strcat(B98, B1C);
            fe_strlwr(B98);
            strcat(B98, "#");
        }
        if (fe_strpos((char *)F + 0x14c, B98)) {
            strcat(B1C, " dollars");
            strcat(B1C, t->punc[i + 2]);
            put_word(&t->word[i + 2], B1C);
            bx = 2;
            mag = 1;
        } else {
            cat_word(&t->word[i + 1], " dollars");
            bx = 1;
        }
    } else if (!strcmp(fe_tail(t->word[i], 1), "?")) {
        /* "$5?": "? dollars" (the "?" before "dollars"), or "$5? million" */
        B1C[0] = 0;
        strncat(B1C, t->word[i] + 1, strlen(t->word[i]) - 2);
        strcat(B1C, " dollars?");
        strcpy(B5C, B1C);
        B1C[0] = 0;
        strcpy(B98, "ZZ!Z");
        if (t->cur_word >= e) {
            B98[0] = 0;
            strncat(B1C, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
            strcat(B98, "#");
            strcat(B98, B1C);
            fe_strlwr(B98);
            strcat(B98, "#");
        }
        if (fe_strpos((char *)F + 0x14c, B98)) {
            strcat(B1C, " dollars");
            strcat(B1C, t->punc[i + 1]);
            put_word(&t->word[i + 1], B1C);
            fe_str_replace_all(B5C, " dollars?", "", B5C);
            bx = 1;
            mag = 1;
            cat_word(&t->word[i + 1], "?");
        } else {
            cat_word(&t->word[i], " dollars");
            bx = 0;
        }
    } else {
        sfx = NULL;
        if (t->cur_word >= e) {
            /* "$5 million" -> "5" + "million dollars" */
            B1C[0] = 0;
            B98[0] = 0;
            strncat(B1C, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
            strcat(B98, "#");
            strcat(B98, B1C);
            fe_strlwr(B98);
            strcat(B98, "#");
            if (fe_strpos((char *)F + 0x14c, B98)) {
                strcat(B1C, " dollars");
                strcat(B1C, t->punc[i + 1]);
                bx = 1;
                mag = 1;
                put_word(&t->word[i + 1], B1C);
                B1C[0] = 0;
                sfx = t->word[i] + 1;
                goto tail;
            }
        }
        w = t->word[i];
        n = (int)strlen(w) - (int)strlen(t->punc[i]);     /* the word without its punctuation */
        if (strlen(t->word_lc) >= 9) {
            /* "$5million" -> "5 million dollars" */
            B98[0] = 0;
            strcat(B98, at(w, n - 6));
            B1C[0] = 0;
            strncat(B1C, B98, strlen(B98) - strlen(t->punc[i]));
            if (!strcmp(B1C, "illion")) {
                B1C[0] = 0;
                strncat(B1C, w + 1, n - 8);
                strcat(B1C, " ");
                B98[0] = 0;
                strcat(B98, at(w, n - 7));
                strncat(B1C, B98, strlen(B98) - strlen(t->punc[i]));
                sfx = " dollars";
            }
        }
        if (!sfx) {
            B98[0] = 0;
            strcat(B98, at(w, n - 1));
            B1C[0] = 0;
            strncat(B1C, B98, strlen(B98) - strlen(t->punc[i]));
            if (!strcmp(B1C, "M") || !strcmp(B1C, "B")) {
                /* "$5M" -> "5 Million dollars" */
                B1C[0] = 0;
                strncat(B1C, w + 1, n - 2);
                strcat(B1C, " ");
                B98[0] = 0;
                strcat(B98, at(w, n - 1));
                strncat(B1C, B98, strlen(B98) - strlen(t->punc[i]));
                sfx = "illion dollars";
            } else {
                B1C[0] = 0;
                strncat(B1C, w + 1, n - 1);
                sfx = " dollars";
            }
        }
        strcat(B1C, sfx);
        sfx = t->punc[i];
        bx = 0;
    tail:
        strcat(B1C, sfx);
        strcpy(B5C, B1C);
    }

    /* "dollars" -> "dollar" when the amount is used as a modifier ("a $5 bill"): the next word is not punctuated or
       separated and is not in the lists; always for "$1" / "$1.xx" (by out_text) */
    {
        char *ot = t->out_text;
        if (!strcmp(ot, "$1") || (!strncmp(ot, "$1.", 3) && strlen(ot) == 5)) goto singular;
        k = I + bx;
        if (t->cur_word == k) goto split;
        if (strlen(t->punc[k]) != 0 || strlen(t->sepr[k]) != 0) goto split;
        if (fe_strpos(ot, "to") || fe_strpos(ot, "per")) goto split;
        k++;
        if (t->cur_word >= k) {
            static const char *const ops[] = {"&", "+", "-", "/", "(", "["};
            for (q = 0; q < 6; q++)
                if (fe_strpos(t->word[k], ops[q])) goto split;
        }
        if (strlen(t->prepunc[k]) != 0 || !strcmp(t->word[k], "per")) goto split;
        strcpy(B1C, "#");
        hashed(B1C, t, k);
        if (fe_strpos((char *)F + 0x170, B1C)) goto split;
        if (fe_strpos(B1C, "$") == 2) goto split;
        for (q = 0; q < sizeof plural_lists / sizeof plural_lists[0]; q++)
            if (fe_strpos((char *)F + plural_lists[q], B1C)) goto split;
    singular:
        if (bx == 0)
            fe_str_replace_all(B5C, "dollars", "dollar", B5C);
        else
            fe_str_replace_all(t->word[I + bx], "dollars", "dollar", t->word[I + bx]);
    }

split:
    /* letters in front of the number: "US5 dollars" -> "5 US dollars", "5million" -> "5 million" */
    bx = fe_strpos(B5C, " ");
    if (bx > 1) {
        for (bp = 1;; ) {
            BD4[0] = 0;
            strncat(BD4, B5C + bp - 1, 1);
            fe_strlwr(BD4);
            if ((signed char)BD4[0] <= 'z' && (signed char)BD4[0] >= 'a') break;
            if (++bp >= bx) goto parts;
        }
        if (bp > 1) {
            B110[0] = 0;
            strncat(B110, B5C, bp - 1);
            strcat(B110, " ");
            strcat(B110, B5C + bp - 1);
            bx = bp;
            strcpy(B5C, B110);
        } else {
            for (bp = 2; bp < bx; bp++) {
                BD4[0] = 0;
                strncat(BD4, B5C + bp - 1, 1);
                if (((signed char)BD4[0] >= '0' && (signed char)BD4[0] <= '9') || BD4[0] == '.') {
                    BD4[0] = 0;
                    strncat(BD4, B5C, bp - 1);
                    B110[0] = 0;
                    strncat(B110, B5C + bp - 1, bx - bp);
                    B1C[0] = 0;
                    strcat(B1C, B5C + bx);
                    B5C[0] = 0;
                    strcat(B5C, B110);
                    strcat(B5C, " ");
                    strcat(B5C, BD4);
                    strcat(B5C, " ");
                    strcat(B5C, B1C);
                    bx = fe_strpos(B5C, " ");
                    break;
                }
            }
        }
    }

parts:
    /* BD4 = the number, B110 = the rest */
    BD4[0] = 0;
    B110[0] = 0;
    if (bx > 0) {
        strncat(BD4, B5C, bx - 1);
        strncat(B110, B5C + bx, strlen(B5C) - bx);
    } else
        strcat(BD4, B5C);

    /* thousands separators for 4 digits or more before the point */
    if (fe_strpos(BD4, ",") == 0) {
        bp = 0;
        for (si = 1; (short)strlen(BD4) >= si; si++) {
            B1C[0] = 0;
            strncat(B1C, BD4 + si - 1, 1);
            if ((signed char)B1C[0] >= '0' && (signed char)B1C[0] <= '9') bp++;
        }
        if (bp >= 3) {
            B1C[0] = 0;
            ax = fe_strpos(BD4, ".");
            if (ax > 4)
                strncat(B1C, BD4, ax - 1);
            else if (ax == 0 && strlen(BD4) > 3)
                strcat(B1C, BD4);
            if (strlen(B1C) > 3) {
                dx = 0;
                B98[0] = 0;
                for (bx = 9; bx >= 0; bx -= 3) {
                    if ((short)strlen(B1C) > dx + bx) {
                        strncat(B98, B1C + dx, strlen(B1C) - bx - dx);
                        if (bx > 0) strcat(B98, ",");
                        dx = (short)(strlen(B1C) - bx);
                    }
                }
                fe_str_replace_all(BD4, B1C, B98, BD4);
            }
        }
    }

    /* The "illion" test only sees a magnitude inside the word: "$2.75 billion" gives "2 75 cent" + "billion dollars".
       With fe_fixes the amount stays "2.75" when the next word took the magnitude. */
    B5C[0] = 0;
    if ((short)strlen(BD4) >= 3 && !strncmp(BD4 + strlen(BD4) - 3, ".", 1) && !fe_strpos(B110, "illion") &&
        !(fe_fixes && mag)) {
        /* dollars and cents */
        B1C[0] = 0;
        B98[0] = 0;
        strncat(B1C, BD4, strlen(BD4) - 3);
        strcat(B98, BD4 + strlen(BD4) - 2);
        if (strlen(B1C) != 0 && (strcmp(B1C, "0") || !strcmp(BD4, "0.00"))) {
            strcat(B5C, B1C);
            strcat(B5C, " ");
            strcat(B5C, B110);
        }
        if (!strcmp(B98, "00")) goto hundreds;
        B1C[0] = 0;
        strcat(B1C, strncmp(B98, "0", 1) ? B98 : B98 + 1);
        bx = (short)atoi(B1C);
        B110[0] = 0;
        k = (int)strlen(B5C) - (int)strlen(t->punc[i]);
        if ((k >= 0 || !fe_fixes) && !strcmp(B5C + k, t->punc[i])) {
            /* the punctuation moves behind the cents */
            strcat(B110, B5C + strlen(B5C) - strlen(t->punc[i]));
            B98[0] = 0;
            strncat(B98, B5C, strlen(B5C) - strlen(t->punc[i]));
            strcpy(B5C, B98);
        }
        if (bx > 1 && (!strcmp(fe_tail(B5C, 1), "s") || strlen(B5C) == 0))
            strcpy(B98, " cents");
        else
            strcpy(B98, " cent");
        if (fe_strpos(B5C, "dollar")) strcat(B5C, " and ");
        strcat(B5C, B1C);
        strcat(B5C, B98);
        strcat(B5C, B110);
    } else {
        strcat(B5C, BD4);
        strcat(B5C, " ");
        strcat(B5C, B110);
    }

hundreds:
    /* "150 dollars" -> "100 50 dollars" (three characters before the space, no point). The original does not check
       for digits: "$abc" -> "a00 bc dollars", "$5,5" -> "500 ,5 dollars"; fe_fixes requires three digits. */
    if (fe_strpos(B5C, "dollar") && fe_strpos(B5C, " ") == 4 && !fe_strpos(B5C, ".") &&
        (!fe_fixes || (isdigit((unsigned char)B5C[0]) && isdigit((unsigned char)B5C[1]) &&
                       isdigit((unsigned char)B5C[2])))) {
        B1C[0] = 0;
        strncat(B1C, B5C, 1);
        strcat(B1C, "00 ");
        B98[0] = 0;
        strncat(B98, B5C + 1, 2);
        if (!strcmp(B98, "00"))
            B98[0] = 0;
        else if (B98[0] == '0') {
            B98[0] = 0;
            strncat(B98, B5C + 2, 1);
        }
        strcat(B1C, B98);
        strcat(B1C, B5C + 3);
        strcpy(B5C, B1C);
    }

    /* the result replaces out_text up to its first space. word_lc and out_text are 60 bytes: without fe_fixes a
       longer result runs on into the fields after them (word_lc into out_text, which is then read) */
    if (fe_fixes) {
        char o[2 * FBUF + 64];
        short p = fe_strpos(t->out_text, " ");
        copy60(t->word_lc, B5C);
        strcpy(o, B5C);
        if (p) strcat(o, t->out_text + p - 1);
        fe_str_replace_all(o, "US", "U.S.", o);
        copy60(t->out_text, o);
    } else {
        char *wl = (char *)t + offsetof(TextProc, word_lc), *ot = (char *)t + offsetof(TextProc, out_text);
        short p;
        strcpy(wl, B5C);
        p = fe_strpos(ot, " ");
        if (p) {
            B1C[0] = 0;
            strcat(B1C, ot + p - 1);
            strcpy(ot, B5C);
            strcat(ot, B1C);
        } else
            strcpy(ot, B5C);
        fe_str_replace_all(ot, "US", "U.S.", ot);
    }
#undef I
}
