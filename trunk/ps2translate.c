/*
 * ps2translate.c
 * Copyright 2009 by Bill Westfield
 *
 * Code for translating raw "keyset 2" (which is the default) keycodes
 * into meaningful (ascii) charactcers.
 *
 * Except for the use of "PROGMEM" for the table, this code should not
 * be very specific to any hardware or operating environment, although
 * it is sensitive to the nuances of the code passed in.  The target
 * environment uses -keycode for release events, and keycode+PS2K_EXTEND
 * for multi-byte keycodes.
 */

#include "ps2keypolled.h"


#ifdef DEBUGMAIN
/*
 * Run in a unix debugging environment instead of on an AVR!
 */
#include <stdio.h>
#define PROGMEM
#define pgm_read_byte(a) *a
#else
/*
 * Normal AVR environment
 */
#include <avr/pgmspace.h>
#endif


unsigned char ps2k_shiftstatus = 0;
#define PS2K_SHIFT 1
#define PS2K_CTRL 2
#define PS2K_SHIFTLIKE 0x80

/*
 * Scrunched up somewhat redundent table of "funny" shifted characters
 */
const unsigned char PROGMEM ps2k_puncttab[] = {
/*  27    28    29    2A    2B    2C    2D    2E    2F     */
/*   '     (     )     *     +     ,     -     .     /     */
    '"',  '(',  ')',  '*',  '+',  '<',  '_',  '>',  '?',
/*  30    31    32    33    34    35    36    37    38     */
/*   0     1     2     3     4     5     6     7     8     */
    ')',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',
/*  39    3A    3B    3C    3D  ******  5B    5C    5D    5E    5F    60  */
/*   9     :     ;     <     =  ******   [     \     ]     ^     _     `  */
    '(',  ':',  ':',  '<',  '+',/*****/ '{',  '|',  '}',  '^',  '_',  '~'
};

const unsigned char PROGMEM ps2k_transtab[] = {
	0,		/* 00 non-US-1 */
	0,		/* 01 F9 */
	0,
	0,		/* 03 F5 */
	0,		/* 04 F3 */
	0,		/* 05 F1 */
	0,		/* 06 F2 */
	0,		/* 07 F12 */
	0,
	0,		/* 09 F10 */
	0,		/* 0a F8 */
	0,		/* 0b F6 */
	0,		/* 0c F4 */
	9,		/* 0d Tab */
	'`',		/* 0e ` ~ */
	0,

	0,		/* 10 */
	0,		/* 11 LAlt */
	0x81,		/* 12 LShift */
	0,
	0x82,		/* 14 LCtrl */
	'q',		/* 15 Q */
	'1',		/* 16 1 ! */
	0,
	0,
	0,
	'z',		/* 1a Z */
	's',		/* 1b S */
	'a',		/* 1c A */
	'w',		/* 1d W */
	'2',		/* 1e 2 @ */
	0,

	0,		/* 20 */
	'c',		/* 21 C */
	'x',		/* 22 X */
	'd',		/* 23 D */
	'e',		/* 24 E */
	'4',		/* 25 4 $ */
	'3',		/* 26 3 # */
	0,
	0,
	' ',		/* 29 space */
	'v',		/* 2a V */
	'f',		/* 2b F */
	't',		/* 2c T */
	'r',		/* 2d R */
	'5',		/* 2e 5 % */
	0,

	0,		/* 30 */
	'n',		/* 31 N */
	'b',		/* 32 B */
	'h',		/* 33 H */
	'g',		/* 34 G */
	'y',		/* 35 Y */
	'6',		/* 36 6 ^ */
	0,
	0,		/* 38 */
	0,
	'm',		/* 3a M */
	'j',		/* 3b J */
	'u',		/* 3c U */
	'7',		/* 3d 7 & */
	'8',		/* 3e 8 * */
	0,

	0,		/* 40 */
	',',		/* 41 , < */
	'k',		/* 42 K */
	'i',		/* 43 I */
	'o',		/* 44 O */
	'0',		/* 45 0 ) */
	'9',		/* 46 9 ( */
	0,
	0,
	'.',		/* 49 . > */
	'/',		/* 4a / ? */
	'l',		/* 4b L */
	';',		/* 4c ; : */
	'p',		/* 4d P */
	'-',		/* 4e - _ */
	0,

	0,		/* 50 */
	0,
	'\'',		/* 52 ' " */
	0,
	'[',		/* 54 [ { */
	'=',		/* 55 = + */
	0,
	0,
	0,		/* 58 CapsLock */
	0x81,		/* 59 RShift */
	10,		/* 5a Enter */
	']',		/* 5b ] } */
	0,
	'\\',		/* 5d \ | */
	0,
	0,

	0,		/* 60 */
	0,
	0,
	0,
	0,		/* 64 */
	0,
	8,		/* 66 Backspace */
	0,
	0,
	'1',		/* 69 KP-1 / End */
	0,
	'4',		/* 6b KP-4 / Left */
	'7',		/* 6c KP-7 / Home */
	0,
	0,
	0,

	'0',		/* 70 KP-0 / Ins */	/* 70 */
	'.',		/* 71 KP-. / Del */
	'2',		/* 72 KP-2 / Down */
	'5',		/* 73 KP-5 */
	'6',		/* 74 KP-6 / Right */
	'8',		/* 75 KP-8 / Up */
	27,		/* 76 Esc */
	0,
	0,
	'+',		/* 79 KP-+ */
	'3',		/* 7A KP-3*/
	'-',		/* 7B KP-- */
	'*',		/* 7C KP=* */
	'9',		/* 7D KP-9 */
};

/*
 * ps2k_translate
 * Translate a keycode event (press or release) into an ascii character.
 * Maintain shift and ctrl state so that we can do uppercase and control
 * characters.  Support the keypad as well.
 * Non-ascii character (function keys, arrows, etc) and keycodes with
 * no translation, return PS2K_NOKEY.  (which is more outside the range of
 * normal ascii than 0x0.  Sigh.)
 */
unsigned char ps2k_translate(int code)
{
    unsigned char result;

    /*
     * Handle some special cases
     */
    if (code == PS2K_EXTEND+0x14)		/* Right CTRL */
	code = 0x14;				/* Treat like left CTRL  */
    else if (code == - (PS2K_EXTEND + 0x14))	/*  (for both press/release) */
	code = -0x14;
    else if (code == PS2K_EXTEND + 0x5A) /* Keypad ENTER */
	code = 0x5A;			 /* like ENTER.  Only press matters. */

    /*
     * Although there are lots of possible code (256 or 512, depending on how
     * you handle the extended keycodes), almost all of the codes that are
     * interesting to us are less than 128.  This makes our table smaller,
     * and we can just return "not know" values for things beyond its end.
     */
    if (code > (int) sizeof(ps2k_transtab)) {
	return PS2K_NOKEY;
    }
    if (code < 0) {
	/*
	 * Handle Key release events.  We only have to do something about
	 * these for the keys that behave like SHIFT keys, so that we
	 * only stay in "shifted" state while the key is actually depressed.
	 */
	code = -code;
	if (code < sizeof(ps2k_transtab)) {
	    result = pgm_read_byte(&ps2k_transtab[code]);
	    if (result & PS2K_SHIFTLIKE) {
		ps2k_shiftstatus &= ~ result;  /* clear shift status */
	    }
	}
	return PS2K_NOKEY;
    }    
    result = pgm_read_byte(&ps2k_transtab[code]);
    if (result == 0) {
	return PS2K_NOKEY;			/* Undefined key */
    }
    if (result & PS2K_SHIFTLIKE) {
	ps2k_shiftstatus |= result;	/* Shift key.  Save status */
	return PS2K_NOKEY;		/*  but otherwise ignore */
    }
    if (ps2k_shiftstatus & PS2K_SHIFT) {
	if (result > 'a' && result < 'z') { /* Simple only works for letters */
	    result &= ~('a' - 'A');		/* Make upper case */
	} else {
	    code = result;
	    /*
	     * obnoxious code to handle shifted numbers and puncuation
	     *  from   ',-./0123456789;=[\]`
	     *  to     "<_>?)!@#$%^&*(:+{|}~
	     *
	     */
	    if (code > '=')
		code -= '[' - '=';  /* Scruch upper punc next to lower */
	    code -= '\'';		  /* subtract lowers unshifted punc */
	    if (code < 0 || code > sizeof(ps2k_puncttab))
		return PS2K_NOKEY;
	    result = pgm_read_byte(&ps2k_puncttab[result]);
	}
    }
    if (ps2k_shiftstatus & PS2K_CTRL)
	result &= 0x1F;

    return result;
}

#ifdef DEBUGMAIN
/*
 * debugging code.  Runs on unix.
 * Did you know that:
 *    int f = -12;
 *    if (f < sizeof(foo))
 * will NOT be true because the compiler ends up "promoting" f to unsigned
 * to make "sizeof" ?  I didn't, till I wrote this code!
 */
int main(void)
{
    int tmp;

    tmp = ps2k_translate(0x2A);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x12);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x2A);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(-0x12);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x2A);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x14);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x2A);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(-0x14);
    printf("'%c' (%d)\n", tmp, tmp);

    tmp = ps2k_translate(0x2A);
    printf("'%c' (%d)\n", tmp, tmp);

}
#endif
