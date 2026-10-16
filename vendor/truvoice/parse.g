#include <stdio.h>
#include <string.h>
#include "parse.h"

static int s2n(const char *s, const char *e) {
	// string to integer
	int n = 0;
	for (; s < e; ++s) n = n * 10 + (*s - '0');
	return n;
}

static void s2s(const char *s, const char *e) {
	// helper function for substring extraction
	// using re2c tagging
	int i = 0;
	while (s < e) {
		attr[i] = *s++;
		i++;
	}
	attr[i] = '\0';
}

static void constructCommand(char l, int num) {
	// escape sequence command constructor
	// surrounded with whitespace
	comm[0] = 0x20;
	comm[1] = 0x1B;
	comm[2] = 0x5B;
	if (num > 999) { num = 999; }
	for (int i = 5; i >= 3; i--) {
		comm[i] = (num % 10) + '0';
		num /= 10;
	}
	comm[6] = l;
	comm[7] = 0x20;
}

static void writeCommand(char* MARKER, char* CURSOR) {
	memset(MARKER-1, 0x20, CURSOR+1-MARKER);			// overwrite found command spot with space.
	strncpy(MARKER-1, comm, sizeof(comm)-1);			// put in the escape sequence with null character taken out.
	MARKER = CURSOR+1;									// advance the cursor
}

// translate text commands into escape sequence commands that TruVoice can understand
void parse(char* YYCURSOR) {
	char *YYPAST;
	char *YYMARKER;
	char *t1, *t2;
	int c = 0;

	/*!stags:re2c format = 'char *@@;\n'; */
	/*!re2c
		re2c:yyfill:enable  = 0;
		re2c:define:YYCTYPE = "unsigned char";
		re2c:tags           = 1;

		// ebnf grammar
		command_start = "{{";
		command_end   = "}}";
		option_end    = [ ]+;
		padding       = [ ]*;
		alpha         = [a-z]+;
		digit         = [0-9]+;

		command_start padding "voice" option_end @t1 alpha @t2 padding command_end {
			s2s(t1, t2);
			if (strcmp(attr, "peter") == 0) {
				constructCommand('V', 0);
			}
			else if (strcmp(attr, "sidney") == 0) {
				constructCommand('V', 1);
			}
			else if (strcmp(attr, "eddie") == 0) {
				constructCommand('V', 2);
			}
			else if (strcmp(attr, "douglas") == 0) {
				constructCommand('V', 3);
			}
			else if (strcmp(attr, "biff") == 0) {
				constructCommand('V', 4);
			}
			else if (strcmp(attr, "amos") == 0) {
				constructCommand('V', 5);
			}
			else if (strcmp(attr, "melvin") == 0) {
				constructCommand('V', 6);
			}
			else if (strcmp(attr, "alex") == 0) {
				constructCommand('V', 7);
			}
			else if (strcmp(attr, "wanda") == 0) {
				constructCommand('V', 8);
			}
			else if (strcmp(attr, "julia") == 0) {
				constructCommand('V', 9);
			}
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "volume" option_end @t1 digit @t2 padding command_end {
			c = s2n(t1, t2);
			if (c < 0) {
				c = 0;
			} else if (c > 16) {
				c = 16;
			}
			constructCommand('a', 16 - c);
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "fast" option_end @t1 digit @t2 padding command_end {
			c = s2n(t1, t2);
			if (c < 0) { c = 0; }
			constructCommand('f', c);
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "rate" option_end @t1 digit @t2 padding command_end {
			c = s2n(t1, t2);
			if (c < 50) {
				c = 50;
			} else if (c > 250) {
				c = 250;
			}
			constructCommand('r', c);
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "pitch" option_end @t1 digit @t2 padding command_end {
			c = s2n(t1, t2);
			if (c < 50) {
				c = 50;
			} else if (c > 400) {
				c = 400;
			}
			constructCommand('p', c);
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "pause" option_end @t1 digit @t2 padding command_end {
			c = s2n(t1, t2);
			if (c < 0) {
				c = 0;
			} else if (c > 250) {
				c = 250;
			}
			constructCommand('s', c);
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "email" option_end @t1 alpha @t2 padding command_end {
			s2s(t1, t2);
			if (strcmp(attr, "off") == 0) {
				constructCommand('X', 0);
			}
			else if (strcmp(attr, "on") == 0) {
				constructCommand('X', 1);
			}
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "speakmode" option_end @t1 alpha @t2 padding command_end {
			s2s(t1, t2);
			if (strcmp(attr, "word") == 0) {
				constructCommand('P', 0);
			}
			else if (strcmp(attr, "normal") == 0) {
				constructCommand('P', 1);
			}
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "style" option_end @t1 alpha @t2 padding command_end {
			s2s(t1, t2);
			if (strcmp(attr, "normal") == 0) {
				constructCommand('c', 0);
			}
			else if (strcmp(attr, "whisper") == 0) {
				constructCommand('c', 1);
			}
			else if (strcmp(attr, "monotone") == 0) {
				constructCommand('c', 2);
			}
			writeCommand(YYMARKER, YYCURSOR);
		}
		command_start padding "phoneme" option_end @t1 alpha @t2 padding command_end {
			s2s(t1, t2);
			if (strcmp(attr, "off") == 0) {
				constructCommand('I', 0);
			}
			else if (strcmp(attr, "on") == 0) {
				constructCommand('I', 1);
			}
			writeCommand(YYMARKER, YYCURSOR);
		}
		* {
			// only exit out of lexer loop if
			// the last (null) character has been reached.
			if (*YYCURSOR == '\0') { return; }
		}
	*/
}

// test program (only for debugging)
/*int main(int argc, const char* argv[]) {
	char text[1024];
	while (1) {
		fgets(text, 1024, stdin);
		parse(text);
		printf("%s\n", text);
	}
	return 0;
}*/
