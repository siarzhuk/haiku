/*
 * rtic: Reduced TermInfo Compiler.
 *
 * Copyright 2013 Siarzhuk Zharski, imker@gmx.li
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>


using namespace std;

static size_t gLine = 0;

enum {
	kExtended = -3,
	kDisabled = -2,
	kNotSet = -1
};

struct Cap {
			enum Type {
				kFlag = 0,
				kNumber,
				kString,
				kTypeCount
			};

		Type					fType;
		bool					fDisabled;
		short					fIndex;
		vector<unsigned char>	fData;

			Cap(short index, bool disabled = false)
				:
				fType(kFlag),
				fDisabled(disabled),
				fIndex(index)
			{
				fData.push_back(disabled ? 0 : 1);
			}

			Cap(short number, short index, bool disabled = false)
				:
				fType(kNumber),
				fDisabled(disabled),
				fIndex(index)
			{
				fData.push_back(0xFF & number);
				fData.push_back(0xFF & number >> 8);
			}

			Cap(vector<unsigned char>& data, short index, bool disabled = false)
				:
				fType(kString),
				fDisabled(disabled),
				fIndex(index)
			{
				fData.assign(data.begin(), data.end());
			}
};

struct Entry {
		string					fNames;
		map<string, Cap>		fCaps;
		vector<string>			fUses;
};

map<string, Entry> gEntries;
map<string, string> gAliases;

#define _countof(_a) (sizeof(_a) / sizeof(_a[0]))


const char* gFlags[] = {
	"bw",		// cub1 wraps from column 0 to last column
	"am",		// terminal has automatic margins
	"xsb",		// beehive (f1=escape, f2=ctrl C)
	"xhp",		// standout not erased by overwriting (hp)
	"xenl",		// newline ignored after 80 cols (concept)
	"eo",		// can erase overstrikes with a blank
	"gn",		// generic line type
	"hc",		// hardcopy terminal
	"km",		// Has a meta key (i.e., sets 8th-bit)
	"hs",		// has extra status line
	"in",		// insert mode distinguishes nulls
	"da",		// display may be retained above the screen
	"db",		// display may be retained below the screen
	"mir",		// safe to move while in insert mode
	"msgr",		// safe to move while in standout mode
	"os",		// terminal can overstrike
	"eslok",	// escape can be used on the status line
	"xt",		// tabs destructive, magic so char (t1061)
	"hz",		// cannot print ~'s (hazeltine)
	"ul",		// underline character overstrikes
	"xon",		// terminal uses xon/xoff handshaking
	"nxon",		// padding will not work, xon/xoff required
	"mc5i",		// printer will not echo on screen
	"chts",		// cursor is hard to see
	"nrrmc",	// smcup does not reverse rmcup
	"npc",		// pad character does not exist
	"ndscr",	// scrolling region is non-destructive
	"ccc",		// terminal can re-define existing colors
	"bce",		// screen erased with background color
	"hls",		// terminal uses only HLS color notation (Tektronix)
	"xhpa",		// only positive motion for hpa/mhpa caps
	"crxm",		// using cr turns off micro mode
	"daisy",	// printer needs operator to change character set
	"xvpa",		// only positive motion for vpa/mvpa caps
	"sam",		// printing in last column causes cr
	"cpix",		// changing character pitch changes resolution
	"lpix",		// changing line pitch changes resolution
	"OTbs",		// uses ^H to move left
	"OTns",		// crt cannot scroll
	"OTnc",		// no way to go to start of line
	"OTMT",		// has meta key
	"OTNL",		// move down with \n
	"OTpt",		// has 8-char tabs invoked with ^I
	"OTxr"		// return clears the line
};

const char* gNumbers[] = {
	"cols",		// number of columns in a line
	"it",		// tabs initially every # spaces
	"lines",	// number of lines on screen or page
	"lm",		// lines of memory if > line. 0 means varies
	"xmc",		// number of blank characters left by smso or rmso
	"pb",		// lowest baud rate where padding needed
	"vt",		// virtual terminal number (CB/unix)
	"wsl",		// number of columns in status line
	"nlab",		// number of labels on screen
	"lh",		// rows in each label
	"lw",		// columns in each label
	"ma",		// maximum combined attributes terminal can handle
	"wnum",		// maximum number of defineable windows
	"colors",	// maximum number of colors on screen
	"pairs",	// maximum number of color-pairs on the screen
	"ncv",		// video attributes that cannot be used with colors
	"bufsz",	// numbers of bytes buffered before printing
	"spinv",	// spacing of pins vertically in pins per inch
	"spinh",	// spacing of dots horizontally in dots per inch
	"maddr",	// maximum value in micro_..._address
	"mjump",	// maximum value in parm_..._micro
	"mcs",		// character step size when in micro mode
	"mls",		// line step size when in micro mode
	"npins",	// numbers of pins in print-head
	"orc",		// horizontal resolution in units per line
	"orl",		// vertical resolution in units per line
	"orhi",		// horizontal resolution in units per inch
	"orvi",		// vertical resolution in units per inch
	"cps",		// print rate in characters per second
	"widcs",	// character step size when in double wide mode
	"btns",		// number of buttons on mouse
	"bitwin",	// number of passes for each bit-image row
	"bitype",	// type of bit-image device
	"OTug",		// number of blanks left by ul
	"OTdC",		// pad needed for CR
	"OTdN",		// pad needed for LF
	"OTdB",		// padding required for ^H
	"OTdT",		// padding required for ^I
	"OTkn"		// count of function keys
};

const char* gStrings[] = {
	"cbt",		// back tab (P)
	"bel",		// audible signal (bell) (P)
	"cr",		// carriage return (P*) (P*)
	"csr",		// change region to line #1 to line #2 (P)
	"tbc",		// clear all tab stops (P)
	"clear",	// clear screen and home cursor (P*)
	"el",		// clear to end of line (P)
	"ed",		// clear to end of screen (P*)
	"hpa",		// horizontal position #1, absolute (P)
	"cmdch",	// terminal settable cmd character in prototype !?
	"cup",		// move to row #1 columns #2
	"cud1",		// down one line
	"home",		// home cursor (if no cup)
	"civis",	// make cursor invisible
	"cub1",		// move left one space
	"mrcup",	// memory relative cursor addressing, move to row #1 columns #2
	"cnorm",	// make cursor appear normal (undo civis/cvvis)
	"cuf1",		// non-destructive space (move right one space)
	"ll",		// last line, first column (if no cup)
	"cuu1",		// up one line
	"cvvis",	// make cursor very visible
	"dch1",		// delete character (P*)
	"dl1",		// delete line (P*)
	"dsl",		// disable status line
	"hd",		// half a line down
	"smacs",	// start alternate character set (P)
	"blink",	// turn on blinking
	"bold",		// turn on bold (extra bright) mode
	"smcup",	// string to start programs using cup
	"smdc",		// enter delete mode
	"dim",		// turn on half-bright mode
	"smir",		// enter insert mode
	"invis",	// turn on blank mode (characters invisible)
	"prot",		// turn on protected mode
	"rev",		// turn on reverse video mode
	"smso",		// begin standout mode
	"smul",		// begin underline mode
	"ech",		// erase #1 characters (P)
	"rmacs",	// end alternate character set (P)
	"sgr0",		// turn off all attributes
	"rmcup",	// strings to end programs using cup
	"rmdc",		// end delete mode
	"rmir",		// exit insert mode
	"rmso",		// exit standout mode
	"rmul",		// exit underline mode
	"flash",	// visible bell (may not move cursor)
	"ff",		// hardcopy terminal page eject (P*)
	"fsl",		// return from status line
	"is1",		// initialization string
	"is2",		// initialization string
	"is3",		// initialization string
	"if",		// name of initialization file
	"ich1",		// insert character (P)
	"il1",		// insert line (P*)
	"ip",		// insert padding after inserted character
	"kbs",		// backspace key
	"ktbc",		// clear-all-tabs key
	"kclr",		// clear-screen or erase key
	"kctab",	// clear-tab key
	"kdch1",	// delete-character key
	"kdl1",		// delete-line key
	"kcud1",	// down-arrow key
	"krmir",	// sent by rmir or smir in insert mode
	"kel",		// clear-to-end-of-line key
	"ked",		// clear-to-end-of-screen key
	"kf0",		// F0 function key
	"kf1",		// F1 function key
	"kf10",		// F10 function key
	"kf2",		// F2 function key
	"kf3",		// F3 function key
	"kf4",		// F4 function key
	"kf5",		// F5 function key
	"kf6",		// F6 function key
	"kf7",		// F7 function key
	"kf8",		// F8 function key
	"kf9",		// F9 function key
	"khome",	// home key
	"kich1",	// insert-character key
	"kil1",		// insert-line key
	"kcub1",	// left-arrow key
	"kll",		// lower-left key (home down)
	"knp",		// next-page key
	"kpp",		// previous-page key
	"kcuf1",	// right-arrow key
	"kind",		// scroll-forward key
	"kri",		// scroll-backward key
	"khts",		// set-tab key
	"kcuu1",	// up-arrow key
	"rmkx",		// leave 'keyboard_transmit' mode
	"smkx",		// enter 'keyboard_transmit' mode
	"lf0",		// label on function key f0 if not f0
	"lf1",		// label on function key f1 if not f1
	"lf10",		// label on function key f10 if not f10
	"lf2",		// label on function key f2 if not f2
	"lf3",		// label on function key f3 if not f3
	"lf4",		// label on function key f4 if not f4
	"lf5",		// label on function key f5 if not f5
	"lf6",		// label on function key f6 if not f6
	"lf7",		// label on function key f7 if not f7
	"lf8",		// label on function key f8 if not f8
	"lf9",		// label on function key f9 if not f9
	"rmm",		// turn off meta mode
	"smm",		// turn on meta mode (8th-bit on)
	"nel",		// newline (behave like cr followed by lf)
	"pad",		// padding char (instead of null)
	"dch",		// delete #1 characters (P*)
	"dl",		// delete #1 lines (P*)
	"cud",		// down #1 lines (P*)
	"ich",		// insert #1 characters (P*)
	"indn",		// scroll forward #1 lines (P)
	"il",		// insert #1 lines (P*)
	"cub",		// move #1 characters to the left (P)
	"cuf",		// move #1 characters to the right (P*)
	"rin",		// scroll back #1 lines (P)
	"cuu",		// up #1 lines (P*)
	"pfkey",	// program function key #1 to type string #2
	"pfloc",	// program function key #1 to execute string #2
	"pfx",		// program function key #1 to transmit string #2
	"mc0",		// print contents of screen
	"mc4",		// turn off printer
	"mc5",		// turn on printer
	"rep",		// repeat char #1 #2 times (P*)
	"rs1",		// reset string
	"rs2",		// reset string
	"rs3",		// reset string
	"rf",		// name of reset file
	"rc",		// restore cursor to position of last save_cursor
	"vpa",		// vertical position #1 absolute (P)
	"sc",		// save current cursor position (P)
	"ind",		// scroll text up (P)
	"ri",		// scroll text down (P)
	"sgr",		// define video attributes #1-#9 (PG9)
	"hts",		// set a tab in every row, current columns
	"wind",		// current window is lines #1-#2 cols #3-#4
	"ht",		// tab to next 8-space hardware tab stop
	"tsl",		// move to status line, column #1
	"uc",		// underline char and move past it
	"hu",		// half a line up
	"iprog",	// path name of program for initialization
	"ka1",		// upper left of keypad
	"ka3",		// upper right of keypad
	"kb2",		// center of keypad
	"kc1",		// lower left of keypad
	"kc3",		// lower right of keypad
	"mc5p",		// turn on printer for #1 bytes
	"rmp",		// like ip but when in insert mode
	"acsc",		// graphics charset pairs, based on vt100
	"pln",		// program label #1 to show string #2
	"kcbt",		// back-tab key
	"smxon",	// turn on xon/xoff handshaking
	"rmxon",	// turn off xon/xoff handshaking
	"smam",		// turn on automatic margins
	"rmam",		// turn off automatic margins
	"xonc",		// XON character
	"xoffc",	// XOFF character
	"enacs",	// enable alternate char set
	"smln",		// turn on soft labels
	"rmln",		// turn off soft labels
	"kbeg",		// begin key
	"kcan",		// cancel key
	"kclo",		// close key
	"kcmd",		// command key
	"kcpy",		// copy key
	"kcrt",		// create key
	"kend",		// end key
	"kent",		// enter/send key
	"kext",		// exit key
	"kfnd",		// find key
	"khlp",		// help key
	"kmrk",		// mark key
	"kmsg",		// message key
	"kmov",		// move key
	"knxt",		// next key
	"kopn",		// open key
	"kopt",		// options key
	"kprv",		// previous key
	"kprt",		// print key
	"krdo",		// redo key
	"kref",		// reference key
	"krfr",		// refresh key
	"krpl",		// replace key
	"krst",		// restart key
	"kres",		// resume key
	"ksav",		// save key
	"kspd",		// suspend key
	"kund",		// undo key
	"kBEG",		// shifted begin key
	"kCAN",		// shifted cancel key
	"kCMD",		// shifted command key
	"kCPY",		// shifted copy key
	"kCRT",		// shifted create key
	"kDC",		// shifted delete-character key
	"kDL",		// shifted delete-line key
	"kslt",		// select key
	"kEND",		// shifted end key
	"kEOL",		// shifted clear-to-end-of-line key
	"kEXT",		// shifted exit key
	"kFND",		// shifted find key
	"kHLP",		// shifted help key
	"kHOM",		// shifted home key
	"kIC",		// shifted insert-character key
	"kLFT",		// shifted left-arrow key
	"kMSG",		// shifted message key
	"kMOV",		// shifted move key
	"kNXT",		// shifted next key
	"kOPT",		// shifted options key
	"kPRV",		// shifted previous key
	"kPRT",		// shifted print key
	"kRDO",		// shifted redo key
	"kRPL",		// shifted replace key
	"kRIT",		// shifted right-arrow key
	"kRES",		// shifted resume key
	"kSAV",		// shifted save key
	"kSPD",		// shifted suspend key
	"kUND",		// shifted undo key
	"rfi",		// send next input char (for ptys)
	"kf11",		// F11 function key
	"kf12",		// F12 function key
	"kf13",		// F13 function key
	"kf14",		// F14 function key
	"kf15",		// F15 function key
	"kf16",		// F16 function key
	"kf17",		// F17 function key
	"kf18",		// F18 function key
	"kf19",		// F19 function key
	"kf20",		// F20 function key
	"kf21",		// F21 function key
	"kf22",		// F22 function key
	"kf23",		// F23 function key
	"kf24",		// F24 function key
	"kf25",		// F25 function key
	"kf26",		// F26 function key
	"kf27",		// F27 function key
	"kf28",		// F28 function key
	"kf29",		// F29 function key
	"kf30",		// F30 function key
	"kf31",		// F31 function key
	"kf32",		// F32 function key
	"kf33",		// F33 function key
	"kf34",		// F34 function key
	"kf35",		// F35 function key
	"kf36",		// F36 function key
	"kf37",		// F37 function key
	"kf38",		// F38 function key
	"kf39",		// F39 function key
	"kf40",		// F40 function key
	"kf41",		// F41 function key
	"kf42",		// F42 function key
	"kf43",		// F43 function key
	"kf44",		// F44 function key
	"kf45",		// F45 function key
	"kf46",		// F46 function key
	"kf47",		// F47 function key
	"kf48",		// F48 function key
	"kf49",		// F49 function key
	"kf50",		// F50 function key
	"kf51",		// F51 function key
	"kf52",		// F52 function key
	"kf53",		// F53 function key
	"kf54",		// F54 function key
	"kf55",		// F55 function key
	"kf56",		// F56 function key
	"kf57",		// F57 function key
	"kf58",		// F58 function key
	"kf59",		// F59 function key
	"kf60",		// F60 function key
	"kf61",		// F61 function key
	"kf62",		// F62 function key
	"kf63",		// F63 function key
	"el1",		// Clear to beginning of line
	"mgc",		// clear right and left soft margins
	"smgl",		// set left soft margin at current column.
	"smgr",		// set right soft margin at current column
	"fln",		// label format
	"sclk",		// set clock, #1 hrs #2 mins #3 secs
	"dclk",		// display clock
	"rmclk",	// remove clock
	"cwin",		// define a window #1 from #2,#3 to #4,#5
	"wingo",	// go to window #1
	"hup",		// hang-up phone
	"dial",		// dial number #1
	"qdial",	// dial number #1 without checking
	"tone",		// select touch tone dialing
	"pulse",	// select pulse dialing
	"hook",		// flash switch hook
	"pause",	// pause for 2-3 seconds
	"wait",		// wait for dial-tone
	"u0",		// User string #0
	"u1",		// User string #1
	"u2",		// User string #2
	"u3",		// User string #3
	"u4",		// User string #4
	"u5",		// User string #5
	"u6",		// User string #6
	"u7",		// User string #7
	"u8",		// User string #8
	"u9",		// User string #9
	"op",		// Set default pair to its original value
	"oc",		// Set all color pairs to the original ones
	"initc",	// initialize color #1 to (#2,#3,#4)
	"initp",	// Initialize color pair #1 to fg=(#2,#3,#4), bg=(#5,#6,#7)
	"scp",		// Set current color pair to #1
	"setf",		// Set foreground color #1
	"setb",		// Set background color #1
	"cpi",		// Change number of characters per inch to #1
	"lpi",		// Change number of lines per inch to #1
	"chr",		// Change horizontal resolution to #1
	"cvr",		// Change vertical resolution to #1
	"defc",		// Define a character #1, #2 dots wide, descender #3
	"swidm",	// Enter double-wide mode
	"sdrfq",	// Enter draft-quality mode
	"sitm",		// Enter italic mode
	"slm",		// Start leftward carriage motion
	"smicm",	// Start micro-motion mode
	"snlq",		// Enter NLQ mode
	"snrmq",	// Enter normal-quality mode
	"sshm",		// Enter shadow-print mode
	"ssubm",	// Enter subscript mode
	"ssupm",	// Enter superscript mode
	"sum",		// Start upward carriage motion
	"rwidm",	// End double-wide mode
	"ritm",		// End italic mode
	"rlm",		// End left-motion mode
	"rmicm",	// End micro-motion mode
	"rshm",		// End shadow-print mode
	"rsubm",	// End subscript mode
	"rsupm",	// End superscript mode
	"rum",		// End reverse character motion
	"mhpa",		// Like column_address in micro mode
	"mcud1",	// Like cursor_down in micro mode
	"mcub1",	// Like cursor_left in micro mode
	"mcuf1",	// Like cursor_right in micro mode
	"mvpa",		// Like row_address #1 in micro mode
	"mcuu1",	// Like cursor_up in micro mode
	"porder",	// Match software bits to print-head pins
	"mcud",		// Like parm_down_cursor in micro mode
	"mcub",		// Like parm_left_cursor in micro mode
	"mcuf",		// Like parm_right_cursor in micro mode
	"mcuu",		// Like parm_up_cursor in micro mode
	"scs",		// Select character set, #1
	"smgb",		// Set bottom margin at current line
	"smgbp",	// Set bottom margin at line #1 or (if smgtp is not given) #2 lines from bottom
	"smglp",	// Set left (right) margin at column #1
	"smgrp",	// Set right margin at column #1
	"smgt",		// Set top margin at current line
	"smgtp",	// Set top (bottom) margin at row #1
	"sbim",		// Start printing bit image graphics
	"scsd",		// Start character set definition #1, with #2 characters in the set
	"rbim",		// Stop printing bit image graphics
	"rcsd",		// End definition of character set #1
	"subcs",	// List of subscriptable characters
	"supcs",	// List of superscriptable characters
	"docr",		// Printing any of these characters causes CR
	"zerom",	// No motion for subsequent character
	"csnm",		// Produce #1'th item from list of character set names
	"kmous",	// Mouse event has occurred
	"minfo",	// Mouse status information
	"reqmp",	// Request mouse position
	"getm",		// Curses should get button events, parameter #1 not documented.
	"setaf",	// Set foreground color to #1, using ANSI escape
	"setab",	// Set background color to #1, using ANSI escape
	"pfxl",		// Program function key #1 to type string #2 and show string #3
	"devt",		// Indicate language/codeset support
	"csin",		// Init sequence for multiple codesets
	"s0ds",		// Shift to codeset 0 (EUC set 0, ASCII)
	"s1ds",		// Shift to codeset 1
	"s2ds",		// Shift to codeset 2
	"s3ds",		// Shift to codeset 3
	"smglr",	// Set both left and right margins to #1, #2.
	"smgtb",	// Sets both top and bottom margins to #1, #2
	"birep",	// Repeat bit image cell #1 #2 times
	"binel",	// Move to next row of the bit image
	"bicr",		// Move to beginning of same row
	"colornm",	// Give name for color #1
	"defbi",	// Define rectangualar bit image region
	"endbi",	// End a bit-image region
	"setcolor",	// Change to ribbon color #1
	"slines",	// Set page length to #1 lines
	"dispc",	// Display PC character #1
	"smpch",	// Enter PC character display mode
	"rmpch",	// Exit PC character display mode
	"smsc",		// Enter PC scancode mode
	"rmsc",		// Exit PC scancode mode
	"pctrm",	// PC terminal options
	"scesc",	// Escape for scancode emulation
	"scesa",	// Alternate escape for scancode emulation
	"ehhlm",	// Enter horizontal highlight mode
	"elhlm",	// Enter left highlight mode
	"elohlm",	// Enter low highlight mode
	"erhlm",	// Enter right highlight mode
	"ethlm",	// Enter top highlight mode
	"evhlm",	// Enter vertical highlight mode
	"sgr1",		// Define second set of video attributes #1-#6
	"slength",	// YI Set page length to #1 hundredth of an inch
	"OTi2",		// secondary initialization string
	"OTrs",		// terminal reset string
	"OTnl",		// use to move down
	"OTbc",		// move left, if not ^H
	"OTko",		// list of self-mapped keycaps
	"OTma",		// map arrow keys rogue(1) motion keys
	"OTG2",		// single upper left
	"OTG3",		// single lower left
	"OTG1",		// single upper right
	"OTG4",		// single lower right
	"OTGR",		// tee pointing right
	"OTGL",		// tee pointing left
	"OTGU",		// tee pointing up
	"OTGD",		// tee pointing down
	"OTGH",		// single horizontal line
	"OTGV",		// single vertical line
	"OTGC",		// single intersection
	"meml",		// lock memory above cursor
	"memu",		// unlock memory
	"box1"		// box characters primary set
};

unsigned char gEscapes[] = {
	'a', '\x07',
	'b', '\x08',
	'E', '\x1B',
	'e', '\x1B',
	'f', '\x0C',
	'l', '\x0A',
	'n', '\x0A',
	'r', '\x0D',
	's', ' ',
	't', '\x09',
	'^', '^',
	'\\', '\\',
	',', ',',
	':', ':',
	'\0', '\0' // the catch for unlisted
};


static inline std::string&
trimLeft(std::string &s)
{
	s.erase(s.begin(), find_if(s.begin(), s.end(),
		not1(ptr_fun<int, int>(isspace))));
	return s;
}


static inline std::string&
trimRight(std::string &s)
{
	s.erase(find_if(s.rbegin(), s.rend(),
		not1(ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}


static inline std::string&
trim(std::string &s)
{
	return trimLeft(trimRight(s));
}


static void
parseNumber(string str, short& result)
{
	result = atoi(str.c_str());
}


static void
parseString(string str, vector<unsigned char>& result)
{
	size_t count = 0;
	unsigned char number = 0;

	size_t pos = 0;
	while ((pos = str.find("%{", pos)) != string::npos) {
		char* end = NULL;
		int number = strtol(str.c_str() + pos + 2, &end, 10);
		if (number > 0x1F && number < 0x7F
			&& end != NULL && *end == '}'
			&& number != '\\') {
			string s("%'");
			s += number;
			s += "'";
			str.replace(pos, end - str.c_str() - pos + 1, s.c_str());
			pos = 0;
		} else
			pos++;
	}

	enum {
		kDefault = 0,
		kEscaped,
		kControl,
		kNumber
	} state = kDefault;

	for (string::iterator i = str.begin(); i != str.end(); i++) {
		unsigned char ch = *i;

		switch (state) {
			case kControl:
				if (ch < 'A' || ch > '_')
					// non-control? than restore initiator
					result.push_back('^');
				else
					ch -= '@';
				result.push_back(ch);
				state = kDefault;
				break;

			case kEscaped:
				if (isdigit(ch)) {
					number = ch - '0';
					count = 1;
					state = kNumber;

				} else {
					for (size_t i = 0;
						i < _countof(gEscapes); i += 2) {
						if (ch == gEscapes[i]) {
							ch = gEscapes[i + 1];
							break;
						}

						if (gEscapes[i] == '\0')
							cerr << gLine << ": unknown escape:" << ch << endl;
					}
					state = kDefault;
					result.push_back(ch);
				}
				break;

			case kNumber:
				if (!isdigit(ch) || count > 2) {
					result.push_back(number > 0 ? number : 0x80); // HACK?!
					state = ch == '\\' ? kEscaped : (ch == '^' ? kControl : kDefault);
					if (state == kDefault)
						result.push_back(ch);
				} else {
					number *= 8;
					number += ch - '0';
					count++;
				}
				break;

			default:
			case kDefault:
				state = ch == '\\' ? kEscaped : (ch == '^' ? kControl : kDefault);
				if (state == kDefault)
					result.push_back(ch);
				break;
		}
	}

	// looks like the number is pending
	if (state == kNumber)
		result.push_back(number > 0 ? number : 0x80); // HACK!!!

	// strings must be \0-terminated
	result.push_back('\0');
}


static void
addFlag(map<string, Entry>::iterator& entry, string& cap)
{
	map<string, Cap>& caps = entry->second.fCaps;
	const char* str = cap.c_str();
	for (size_t i = 0; i < _countof(gFlags); i++) {
		if (strcmp(str, gFlags[i]) == 0) {
			caps.insert(pair<string, Cap>(cap, Cap(i)));
			return;
		}
	}

	caps.insert(pair<string, Cap>(cap, Cap(kExtended)));
}


static void
addNumber(map<string, Entry>::iterator& entry, string& cap, size_t off)
{
	short num = 0;
	parseNumber(cap.substr(off), num);
	map<string, Cap>& caps = entry->second.fCaps;
	string name = cap.substr(0, off - 1);
	const char* str = name.c_str();
	for (size_t i = 0; i < _countof(gNumbers); i++) {
		if (strcmp(str, gNumbers[i]) == 0) {
			caps.insert(pair<string, Cap>(name, Cap(num, (short)i)));
			return;
		}
	}

	caps.insert(pair<string, Cap>(name, Cap(num, (short)kExtended)));
}


static void
addString(map<string, Entry>::iterator& entry, string& cap, size_t off)
{
	vector<unsigned char> stringData;
	parseString(cap.substr(off), stringData);
	map<string, Cap>& caps = entry->second.fCaps;
	string name = cap.substr(0, off - 1);
	const char* str = name.c_str();
	for (size_t i = 0; i < _countof(gStrings); i++) {
		if (strcmp(str, gStrings[i]) == 0) {
			caps.insert(pair<string, Cap>(name, Cap(stringData, (short)i)));
			return;
		}
	}

	if (strcmp(str, "use") == 0) {
		vector<string>& uses = entry->second.fUses;
		uses.push_back(cap.substr(off));
		return;
	}

	caps.insert(pair<string, Cap>(name, Cap(stringData, (short)kExtended)));
}


static void
addDisable(map<string, Entry>::iterator& entry, string& cap, size_t off)
{
	map<string, Cap>& caps = entry->second.fCaps;
	string name = cap.substr(0, off);
	const char* str = name.c_str();
	for (size_t i = 0; i < _countof(gFlags); i++) {
		if (strcmp(str, gFlags[i]) == 0) {
			caps.insert(pair<string, Cap>(name, Cap(i, true)));
			return;
		}
	}

	for (size_t i = 0; i < _countof(gNumbers); i++) {
		if (strcmp(str, gNumbers[i]) == 0) {
			caps.insert(pair<string, Cap>(name, Cap((short)kDisabled, i, true)));
			return;
		}
	}

	for (size_t i = 0; i < _countof(gStrings); i++) {
		if (strcmp(str, gStrings[i]) == 0) {
			vector<unsigned char> data;
			caps.insert(pair<string, Cap>(name, Cap(data, (short)i, true)));
			return;
		}
	}

	// extended caps disables are always treated as flags
	caps.insert(pair<string, Cap>(name, Cap(kExtended, true)));
}


static void
parseLine(map<string, Entry>::iterator& entry, string& line)
{
	// split at unescaped commas
	bool escaped = false;
	size_t off = 0;
	vector<string> caps;
	for (size_t i = 0; i < line.length(); i++) {
		switch (line[i]) {
			case '^':
			case '\\':
				escaped = !escaped;
				continue;
			case ',':
				if (!escaped) {
					string s = line.substr(off, i - off);
					s = trim(s);
					if (s[0] != '.')
						caps.push_back(s);
					off = i + 1;
				}
			default:
				escaped = false;
				break;
		}
	}

	// detect and parse
	off = 0;
	for (vector<string>::iterator c = caps.begin();
		c != caps.end(); c++) {
		string& s = *c;
		for (size_t i = 0; ; i++) {
			if (i < s.length()) {
				char ch = s[i];
				if (ch == '#')
					addNumber(entry, s, i + 1);
				else if (ch == '=')
					addString(entry, s, i + 1);
				else if (ch == '@')
					addDisable(entry, s, i);
				else
					continue;
			} else
				addFlag(entry, s);
			break;
		}
	}
}


static void
mergeEntries(map<string, Entry>::iterator src,
	map<string, Entry>::iterator dst)
{
	if (src == dst)
		return;

	Entry& srcEntry = src->second;
	Entry& dstEntry = dst->second;

	for (map<string, Cap>::iterator s = srcEntry.fCaps.begin();
		s != srcEntry.fCaps.end(); s++) {
		map<string, Cap>::iterator d = dstEntry.fCaps.find(s->first);
		if (d == dstEntry.fCaps.end())
			dstEntry.fCaps.insert(*s);
	}
}


static void
resolveUses(map<string, Entry>::iterator entry,
		map<string, Entry>::iterator parent)
{
	for (vector<string>::iterator i = entry->second.fUses.begin();
		i != entry->second.fUses.end(); i++) {
		map<string, Entry>::iterator e = gEntries.find(*i);
		if (e == gEntries.end()) {
			map<string, string>::iterator a = gAliases.find(*i);
			if (a != gAliases.end())
				e = gEntries.find(a->second);

			if (e == gEntries.end()) {
				cerr << "Entry '" << entry->first.c_str() << "': "
					<< "unresolved link to '" << i->c_str()
					<< "' ignored." << endl;
				continue;
			}
		}

		resolveUses(e, entry);
	}

	mergeEntries(entry, parent);
}


static bool
mkdirIfNeeded(const char* dirName)
{
	struct stat sb;
	if (stat(dirName, &sb) != 0) {
		if (errno == ENOENT
			&& mkdir(dirName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
				cerr << dirName << " cannot be created: "
					<< strerror(errno) << endl;
			return false;
		}

	} else {
		if (S_ISDIR(sb.st_mode) == 0) {
			cerr << dirName << " already exists and is not directory." << endl;
			return false;
		}

		// let the jam know about the real update time
		utime(dirName, NULL);
	}

	return true;
}


static void
padToWord(ofstream& ofs)
{
	if (ofs.tellp() % 2 != 0)
		ofs.put(char(0));
}


static void
updateExtendedCaps(ofstream& ofs, Entry& entry)
{
	enum {
		kExFlags = 0,
		kExNumbers = 1,
		kExStrings = 2,
		kExStrCount = 3,
		kExTabSize = 4,
		kExHdrSize = 5
	};

	// extended header
	short exHeader[kExHdrSize] = { 0 };

	vector<char> flags;
	vector<char> numbers;
	vector<char> offsets;
	vector<unsigned char> strings;
	vector<char> nameOffsets;
	vector<unsigned char> names;

	for (int t = Cap::kFlag; t != Cap::kTypeCount; t++) {
		for (map<string, Cap>::iterator i = entry.fCaps.begin();
			i != entry.fCaps.end(); i++) {
			Cap& c = i->second;
			if (c.fIndex == kExtended && !c.fDisabled && t == c.fType) {
				switch (c.fType) {
					case Cap::kFlag:
						exHeader[kExFlags]++;
						flags.push_back(1);
						break;
					case Cap::kNumber:
						exHeader[kExNumbers]++;
						numbers.push_back(c.fData[0]);
						numbers.push_back(c.fData[1]);
						break;
					case Cap::kString:
					{
						exHeader[kExStrings]++;
						short offset = strings.size();
						offsets.push_back(0xFF & offset);
						offsets.push_back(0xFF & offset >> 8);
						strings.insert(strings.end(),
							c.fData.begin(), c.fData.end());
						break;
					}
					default:
						// skip adding cap name for unknown type
						continue;
				}
				short offset = names.size();
				nameOffsets.push_back(0xFF & offset);
				nameOffsets.push_back(0xFF & offset >> 8);
				names.insert(names.end(), i->first.begin(), i->first.end());
				names.push_back(0); // inject the terminator
			}
		}
	}

	exHeader[kExStrCount] = exHeader[kExFlags] + exHeader[kExNumbers]
		+ exHeader[kExStrings] * 2;
	exHeader[kExTabSize] = strings.size() + names.size();

	if (exHeader[kExTabSize] == 0)
		return;

	padToWord(ofs);

	for (size_t i = 0; i < _countof(exHeader); i++) {
		ofs.put(0xFF & exHeader[i]);
		ofs.put(0xFF & exHeader[i] >> 8);
	}

	ofs.write(&flags[0], flags.size());

	padToWord(ofs);

	ofs.write(&numbers[0], numbers.size());
	ofs.write(&offsets[0], offsets.size());
	ofs.write(&nameOffsets[0], nameOffsets.size());
	ofs.write((char*)&strings[0], strings.size());
	ofs.write((char*)&names[0], names.size());
}


static void
updateDB(const char* outFolder)
{
	// update term files
	for (map<string, Entry>::iterator e = gEntries.begin();
		e != gEntries.end(); e++) {

		string path(outFolder);
		path += "/";
		path += e->first[0];

		if (!mkdirIfNeeded(path.c_str()))
			continue;

		path += "/";
		path += e->first;
		ofstream ofs(path.c_str(), ios::out | ios::trunc);
		if (!ofs.is_open()) {
			cerr << "Cannot open file " << path.c_str()
				<< " for updating." << endl;
			continue;
		}

		Entry& entry = e->second;

		enum {
			kFlags = 2,
			kNumbers = 3,
			kOffsets = 4,
			kTabSize = 5,
			kHdrSize = 6
		};

		// standard header section
		short header[kHdrSize] = {
			0432,						// magic
			entry.fNames.length() + 1,	// names section including \0
			0  // flags, numbers, string offsets, size of string table
		};

		vector<char> flags(_countof(gFlags), 0);
		vector<char> numbers(_countof(gNumbers) * 2, kNotSet);
		map<short, vector<unsigned char> > strings;

		for (map<string, Cap>::iterator i = entry.fCaps.begin();
			i != entry.fCaps.end(); i++) {
			Cap& c = i->second;
			if (c.fIndex >= 0 && !c.fDisabled) {
				switch (c.fType) {
					case Cap::kFlag:
						header[kFlags]
							= max<short>(header[kFlags], c.fIndex + 1);
						flags[c.fIndex] = c.fData[0];
						break;
					case Cap::kNumber:
						header[kNumbers]
							= max<short>(header[kNumbers], c.fIndex + 1);
						numbers[c.fIndex * 2] = c.fData[0];
						numbers[c.fIndex * 2 + 1] = c.fData[1];
						break;
					case Cap::kString:
						strings.insert(pair<short, vector<unsigned char> >(
								c.fIndex, c.fData));
						break;
					default:
						break;
				}
			}
		}

		vector<char> offsets(_countof(gStrings) * 2, kNotSet);
		vector<char> stringTable;
		for (map<short, vector<unsigned char> >::iterator i = strings.begin();
			i != strings.end(); i++) {
			size_t offset = stringTable.size();
			size_t size = i->second.size();
			header[kOffsets] = max<short>(header[kOffsets], i->first + 1);
			header[kTabSize] = offset + size;
			offsets[i->first * 2] = 0xFF & offset;
			offsets[i->first * 2 + 1] = 0xFF & offset >> 8;
			stringTable.insert(stringTable.end(),
				i->second.begin(), i->second.end());
		}

		for (size_t i = 0; i < _countof(header); i++) {
			ofs.put(0xFF & header[i]);
			ofs.put(0xFF & header[i] >> 8);
		}

		// names and aliases section
		ofs.write(entry.fNames.c_str(), entry.fNames.length() + 1);

		// flags section
		ofs.write(&flags[0], header[kFlags]);
		padToWord(ofs);

		ofs.write(&numbers[0], header[kNumbers] * 2);
		ofs.write(&offsets[0], header[kOffsets] * 2);
		ofs.write(&stringTable[0], header[kTabSize]);

		updateExtendedCaps(ofs, entry);

		ofs.close();
	}

	// update aliases too
	for (map<string, string>::iterator a = gAliases.begin();
		a != gAliases.end(); a++) {

		string path(outFolder);
		path += "/";
		path += a->first[0];

		if (!mkdirIfNeeded(path.c_str()))
			continue;

		path += "/";
		path += a->first;

		string link;
		if (a->first[0] != a->second[0]) {
			link += "../";
			link += a->second[0];
			link += "/";
		}

		link += a->second.c_str();

		if (symlink(link.c_str(), path.c_str()) != 0 && errno != EEXIST) {
			cerr << "Cannot create alias " << a->first.c_str()
				<< " for " << a->second.c_str() << ". "
				<< strerror(errno) << endl;
			continue;
		}
	}
}


int
main(int argc, char** argv)
{
	if (argc < 3) {
		cerr << endl << "Reduced terminfo database compiler" << endl
			<< "\tUsage: rtic < terminfo.src | - > < output dir name >"
			<< endl << endl;
		return -1;
	}

	const char* inFile = argv[1];
	bool useStdIn = strcmp(inFile, "-") == 0;
	ifstream ifs;
	if (!useStdIn) {
		ifs.open(inFile);
		if (!ifs.is_open()) {
			cerr << "Cannot open file " << inFile << " for reading." << endl;
			return -1;
		}
	}

	istream& in = useStdIn ? cin : (istream&)ifs;

	// check output folder before processing
	const char* outFolder = argv[2];
	if (!mkdirIfNeeded(outFolder))
		return -1;

	// start parsing terminfo.src
	string line;
	map<string, Entry>::iterator currentEntry = gEntries.end();

	for (gLine = 1; ; gLine++) {
		getline(in, line);
		if (in.eof())
			break;

		if (line.length() == 0 || line[0] == '#')
			continue;

		if (isspace(line[0])) {
			if (currentEntry == gEntries.end()) {
				cerr << "line " << gLine << ": orphaned line ignored." << endl;
				continue;
			}

			parseLine(currentEntry, line);
			continue;
		}

		size_t pos = line.find_first_of('|');
		if (pos == string::npos)
			pos = line.length() - 1;

		string name = line.substr(0, pos);
		pair<map<string, Entry>::iterator, bool>
			p = gEntries.insert(pair<string, Entry>(name, Entry()));

		if (p.second == false) {
			currentEntry = gEntries.end();
			cerr << "line " << gLine << ": duplicate terminfo entry ignored:"
				<< name.c_str() << endl;
			// force to ignore it's caps too
			currentEntry = gEntries.end();
			continue;
		}

		currentEntry = p.first;
		currentEntry->second.fNames = line.substr(0, line.length() - 1);

		for (size_t end = 0;
			(end = line.find_first_of('|', ++pos)) != string::npos; pos = end)
			gAliases.insert(
				pair<string, string>(line.substr(pos, end - pos), name));
	}

	ifs.close();

	// resolve "use" capabilities and normalize string offsets
	for (map<string, Entry>::iterator e = gEntries.begin();
		e != gEntries.end(); e++)
		resolveUses(e, e);

	// setup / update the terminfo database
	updateDB(outFolder);

	return 0;
}

