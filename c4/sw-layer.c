// sw-layer.c -- c4's platform layer for JFBuild (baselayer.h), in place of
// its SDL one (jfbuild/src/sdlayer2.c), which it follows function by
// function. Every build has this file: c4lc's for C4IX, C4DOS and the bare
// machine, and gcc's for the host. What differs between them is
// underneath, in the platform (sw_plat_*): libjs's display (sw-gui.c), or
// nothing at all (sw-host.c).
//
// The frame is an 8-bit buffer of xdim x ydim that the engine draws in,
// sent whole with its palette when the game shows it. Any size in the mode
// list below can be chosen in the game (Options > Screen Setup), and the
// display scales it to fill the screen. Keys arrive as a browser's key
// codes and become the PC scan codes Build reads, with the character
// they type for the console and the menus; the mouse moves by its motion,
// relative, as SDL's grabbed mouse did.
//
// The clock is the machine's, in milliseconds, but in a test (-sum) it is
// one that moves only as the game runs (sw-sum.c), so that every build
// sees the same times and draws the same frames.
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman; see
// BUILDLIC.TXT. This file is not part of Ken Silverman's original release.

#include "build.h"
#include "baselayer.h"
#include "baselayer_priv.h"
#include "osd.h"
#include "a.h"
#include "sw-plat.h"

int displaycnt = 1;

// JFBuild's version.c, which its build writes from git: c4's images are
// the same bytes each time they are built, so the build is named instead
const char *build_version = "c4";
const char *build_date = "games/sw";
const char *build_time = "";

static unsigned char *frame;		// xres x yres, 8 bits a pixel
static int pal32[256];			// 0x00RRGGBB, for the display
static char mouseacquired, moustat;
static int pages;			// frames shown, for handleevents' held keys

// ---- the program ----

int main (int argc, char *argv[])
{
	int r;
	sw_sum_args (argc, argv);
	_buildargc = argc;
	_buildargv = (const char **)argv;
	sw_plat_init (argc, argv);
	startwin_open ();
	baselayer_init ();
	r = app_main (argc, (char const * const *)argv);
	startwin_close ();
	sw_quit ();
	return r;
}

int initsystem (void)
{
	atexit (uninitsystem);
	return 0;
}

void uninitsystem (void)
{
	uninitinput ();
	uninittimer ();
}

void sw_quit (void)
{
	sw_plat_close ();
}

// what the game says as it starts: stdout already has it (buildputs)
void initputs (const char *str)
{
	(void)str;
}

void debugprintf (const char *f, ...)
{
	(void)f;
}

// ---- the window manager: there is none ----

int wm_msgbox (const char *name, const char *fmt, ...)
{
	va_list va;
	if (name)
		printf ("%s: ", name);
	va_start (va, fmt);
	vprintf (fmt, va);
	va_end (va);
	printf ("\n");
	return 0;
}

int wm_ynbox (const char *name, const char *fmt, ...)
{
	va_list va;
	if (name)
		printf ("%s: ", name);
	va_start (va, fmt);
	vprintf (fmt, va);
	va_end (va);
	printf (" (yes)\n");
	return 1;
}

int wm_filechooser (const char *initialdir, const char *initialfile, const char *type, int foropen, char **choice)
{
	(void)initialdir; (void)initialfile; (void)type; (void)foropen; (void)choice;
	return 0 - 1;
}

int wm_idle (void *ptr) { (void)ptr; return 0; }
void wm_setapptitle (const char *name) { (void)name; }
void wm_setwindowtitle (const char *name) { (void)name; }
void wm_allowbackgroundidle (int onf) { (void)onf; }
void wm_allowtaskswitching (int onf) { (void)onf; }

// ---- keys ----

// A browser's key code as the PC's scan code, as Build numbers them: an
// extended key (0xE0 nn) is 0x80 + nn, as SDL's layer made them. 0 for a
// key Build has no use for. Duke Nukem 3D's port has the same table.
static unsigned short scancodes[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0-7
	0x0E, 0x0F, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00,	// 8-15: backspace tab, enter
	0x2A, 0x1D, 0x38, 0x59, 0x3A, 0x00, 0x00, 0x00,	// 16-23: shift ctrl alt pause caps
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,	// 24-31: esc
	0x39, 0xC9, 0xD1, 0xCF, 0xC7, 0xCB, 0xC8, 0xCD,	// 32-39: space pgup pgdn end home arrows
	0xD0, 0x00, 0x00, 0x00, 0x00, 0xD2, 0xD3, 0x00,	// 40-47: down, insert delete
	0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,	// 48-55: 0-7
	0x09, 0x0A, 0x00, 0x27, 0x00, 0x0D, 0x00, 0x00,	// 56-63: 8 9 ; =
	0x00, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22,	// 64-71: a-g
	0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,	// 72-79: h-o
	0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11,	// 80-87: p-w
	0x2D, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00,	// 88-95: x y z
	0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47,	// 96-103: keypad 0-7
	0x48, 0x49, 0x37, 0x4E, 0x00, 0x4A, 0x53, 0xB5,	// 104-111: keypad 8 9 * + - . /
	0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42,	// 112-119: F1-F8
	0x43, 0x44, 0x57, 0x58, 0x00, 0x00, 0x00, 0x00,	// 120-127: F9-F12
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 128-135
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 136-143
	0x45, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 144-151: num lock, scroll lock
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 152-159
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 160-167
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00,	// 168-175: - (Firefox)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 176-183
	0x00, 0x00, 0x27, 0x0D, 0x33, 0x0C, 0x34, 0x35,	// 184-191: ; = , - . /
	0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 192-199: `
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 200-207
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 208-215
	0x00, 0x00, 0x00, 0x1A, 0x2B, 0x1B, 0x28, 0x00,	// 216-223: [ \ ] '
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 224-231
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 232-239
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 240-247
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 248-255
};

// The character a key types, on a US keyboard, for the console and the
// menus' text (a saved game's name): by its scan code, unshifted and
// shifted. Enter, Backspace, Tab and Esc are the control characters
// SDL's layer put in the same queue.
static const char *plain =
	"\0\033" "1234567890-=\b\t" "qwertyuiop[]\r\0" "asdfghjkl;'`\0\\" "zxcvbnm,./\0*\0 ";
static const char *shifted =
	"\0\033" "!@#$%^&*()_+\b\t" "QWERTYUIOP{}\r\0" "ASDFGHJKL:\"~\0|" "ZXCVBNM<>?\0*\0 ";

static int typed (int sc)
{
	int shift;
	if (sc == 0x9C)
		return 13;			// keypad enter
	if (sc == 0xB5)
		return '/';
	if (sc >= 0x47 && sc <= 0x53)
		return "789-456+1230."[sc - 0x47];
	if (sc >= 0x3A)
		return 0;
	shift = keystatus[0x2A] || keystatus[0x36];
	if (keystatus[0x1D] || keystatus[0x9D])
	{
		// Ctrl and a letter: its control character, as SDL's layer gave
		char c = plain[sc];
		return c >= 'a' && c <= 'z' ? c - 'a' + 1 : 0;
	}
	return (unsigned char)(shift ? shifted[sc] : plain[sc]);
}

static int eattext;

static void ascii (int c)
{
	if (c && OSD_HandleChar (c))
		if (((keyasciififoend + 1) & (KEYFIFOSIZ - 1)) != keyasciififoplc)
		{
			keyasciififo[keyasciififoend] = (unsigned char)c;
			keyasciififoend = (keyasciififoend + 1) & (KEYFIFOSIZ - 1);
		}
}

// a key to Build, as SDL's layer gave it: the console first, then the
// game's state and queue, then the character it types
static void key_sc (int sc, int down, int ch)
{
	if (sc == 0)
		return;
	if (sc == OSD_CaptureKey (0 - 1))
	{
		if (down)
			OSD_ShowDisplay (0 - 1);	// and the ` it types is eaten
		return;
	}
	if (OSD_HandleKey (sc, down) == 0)
		return;
	keystatus[sc] = down ? 1 : 0;
	keyfifo[keyfifoend] = sc;
	keyfifo[(keyfifoend + 1) & (KEYFIFOSIZ - 1)] = down ? 1 : 0;
	keyfifoend = (keyfifoend + 2) & (KEYFIFOSIZ - 1);
	if (down)
		ascii (ch ? ch : typed (sc));
}

static int key_scan (int code)
{
	return code >= 0 && code < 256 ? scancodes[code] : 0;
}

void sw_key (int code, int down)
{
	key_sc (key_scan (code), down, 0);
}

static char keynames[256][16];

static void name_keys (void)
{
	static const char *names[] = {
		"\x01" "Escape", "\x0e" "Backspace", "\x0f" "Tab", "\x1c" "Enter", "\x1d" "LCtrl",
		"\x2a" "LShift", "\x36" "RShift", "\x38" "LAlt", "\x39" "Space", "\x3a" "CapsLck",
		"\x45" "NumLock", "\x46" "ScrLock", "\x37" "Kpad*", "\x4a" "Kpad-", "\x4e" "Kpad+",
		"\x47" "Kpad7", "\x48" "Kpad8", "\x49" "Kpad9", "\x4b" "Kpad4", "\x4c" "Kpad5",
		"\x4d" "Kpad6", "\x4f" "Kpad1", "\x50" "Kpad2", "\x51" "Kpad3", "\x52" "Kpad0",
		"\x53" "Kpad.", "\x57" "F11", "\x58" "F12", "\x59" "Pause", "\x9c" "KpdEnter",
		"\x9d" "RCtrl", "\xb5" "Kpad/", "\xb8" "RAlt", "\xc7" "Home", "\xc8" "Up",
		"\xc9" "PgUp", "\xcb" "Left", "\xcd" "Right", "\xcf" "End", "\xd0" "Down",
		"\xd1" "PgDn", "\xd2" "Insert", "\xd3" "Delete", 0
	};
	int i, c;
	for (i = 0; names[i]; i++)
		strcpy (keynames[(unsigned char)names[i][0]], names[i] + 1);
	for (i = 0x3B; i <= 0x44; i++)
		sprintf (keynames[i], "F%d", i - 0x3A);
	for (i = 1; i < 0x3A; i++)
		if (!keynames[i][0] && (c = (unsigned char)shifted[i]) > ' ')
		{
			keynames[i][0] = (char)(plain[i] >= 'a' && plain[i] <= 'z' ? c : plain[i]);
			keynames[i][1] = 0;
		}
}

int initinput (void)
{
	inputdevices = 1 | 2;		// keyboard (1) and mouse (2)
	mouseacquired = 0;
	name_keys ();
	return 0;
}

void uninitinput (void)
{
	uninitmouse ();
}

const char *getkeyname (int num)
{
	if ((unsigned)num >= 256)
		return NULL;
	return keynames[num];
}

const char *getjoyname (int what, int num)
{
	(void)what; (void)num;
	return NULL;
}

// ---- the mouse ----

int initmouse (void)
{
	moustat = 1;
	grabmouse (1);
	return 0;
}

void uninitmouse (void)
{
	grabmouse (0);
	moustat = 0;
}

void grabmouse (int a)
{
	if (a != mouseacquired)
		sw_plat_grab (a && moustat);
	mouseacquired = (char)a;
	mousex = mousey = 0;
}

void readmousexy (int *x, int *y)
{
	if (!mouseacquired || !moustat)
	{
		*x = *y = 0;
		return;
	}
	*x = mousex;
	*y = mousey;
	mousex = mousey = 0;
}

void readmousebstatus (int *b)
{
	if (!mouseacquired || !moustat)
		*b = 0;
	else
		*b = mouseb;
	// the wheel's turn is a click: the game has it now
	mouseb &= ~(1 << 4 | 1 << 5);
}

void releaseallbuttons (void)
{
}

// the mouse's buttons as SDL's layer numbered them: left, right, middle
void sw_mouse_button (int button, int down)
{
	int bit;
	bit = button == 0 ? 1 : button == 2 ? 2 : 4;
	if (down)
		mouseb |= bit;
	else
		mouseb &= ~bit;
}

void sw_mouse_wheel (int up)
{
	mouseb |= up ? 1 << 5 : 1 << 4;
}

void sw_mouse_move (int dx, int dy)
{
	if (mouseacquired)
	{
		mousex += dx;
		mousey += dy;
	}
}

// ---- events ----

// A key or button let go in the frame it went down in is let go only after
// the whole of the next frame: the game asks what is down once a frame,
// and a tap quicker than a frame would otherwise never be seen down (as
// Duke Nukem 3D's port does).
enum { HELD = 16 };
static int held[HELD], heldat[HELD], nheld;	// scan codes, or 1000 + a button
static int downat[256 + 8];			// the frame each went down in

static void let_go (int what)
{
	if (what >= 1000)
		sw_mouse_button (what - 1000, 0);
	else
		key_sc (what, 0, 0);
}

static void hold (int what, int at)
{
	if (nheld < HELD)
	{
		held[nheld] = what;
		heldat[nheld++] = at;
	}
	else
		let_go (what);
}

int handleevents (void)
{
	int ev[4], i, n, sc, b, rv;
	rv = 0;
	sw_snd_poll ();
	sw_sum_input ();
	// down in frame k: still down all through frame k + 1, and let go
	// once pages is k + 2
	for (i = n = 0; i < nheld; i++)
		if (heldat[i] < pages - 1)
			let_go (held[i]);
		else
		{
			held[n] = held[i];
			heldat[n++] = heldat[i];
		}
	nheld = n;
	while (sw_plat_poll (ev))
	{
		switch (ev[0])
		{
		case SW_EV_KEYDOWN:
			sc = key_scan (ev[1]);
			downat[sc] = pages;
			// the display's own character when it has one, but for the
			// control characters, which come from the scan code
			key_sc (sc, 1, ev[2] >= 32 && ev[2] < 127 ? ev[2] : 0);
			break;
		case SW_EV_KEYUP:
			sc = key_scan (ev[1]);
			if (sc && downat[sc] >= pages - 1)
				hold (sc, downat[sc]);
			else
				key_sc (sc, 0, 0);
			break;
		case SW_EV_REL:
			sw_mouse_move (ev[1], ev[2]);
			break;
		case SW_EV_DOWN:
			b = ev[3] & 7;
			downat[256 + b] = pages;
			sw_mouse_button (b, 1);
			break;
		case SW_EV_UP:
			b = ev[3] & 7;
			if (downat[256 + b] >= pages - 1)
				hold (1000 + b, downat[256 + b]);
			else
				sw_mouse_button (b, 0);
			break;
		case SW_EV_WHEEL:
			sw_mouse_wheel (ev[3] < 0);
			break;
		case SW_EV_QUIT:
			quitevent = 1;
			rv = 0 - 1;
			break;
		}
	}
	sampletimer ();
	return rv;
}

// ---- the timer: Ken's idea, as SDL's layer had it ----

static int timerticspersec;
static int timerlastsample;
static void (*usertimercallback) (void);

// milliseconds as timer ticks, in 32 bits
static int ms_ticks (uint32_t ms)
{
	return (int)((ms / 1000) * timerticspersec + (ms % 1000) * timerticspersec / 1000);
}

int inittimer (int tickspersecond, void (*callback) (void))
{
	if (timerticspersec)
		return 0;
	timerticspersec = tickspersecond;
	timerlastsample = ms_ticks (sw_ms ());
	usertimercallback = callback;
	return 0;
}

void uninittimer (void)
{
	timerticspersec = 0;
}

void sampletimer (void)
{
	int n;
	// the game's waits for a sound to end come through here: on DOS the
	// card's interrupt ended it, here the poll does
	sw_snd_poll ();
	if (!timerticspersec)
		return;
	n = ms_ticks (sw_ms ()) - timerlastsample;
	if (n > 0)
	{
		totalclock += n;
		timerlastsample += n;
	}
	if (usertimercallback)
		for (; n > 0; n--)
			usertimercallback ();
}

unsigned int getticks (void)
{
	return sw_ms ();
}

unsigned int getusecticks (void)
{
	return sw_ms () * 1000;
}

int gettimerfreq (void)
{
	return timerticspersec;
}

// ---- video ----

// The sizes the game offers. The display scales any of them to the
// screen; the game draws a bigger one sharper, at a cost in time that
// grows with its pixels. 320x200 is what the DOS game ran in.
static int stdres[][2] = {
	{320, 200}, {320, 240}, {400, 300}, {512, 384},
	{640, 400}, {640, 480}, {800, 600}, {0, 0}
};

void getvalidmodes (void)
{
	int i;
	if (validmodecnt)
		return;
	// full screen and windowed alike: the game has the display, or the
	// window it was given, either way
	for (i = 0; stdres[i][0]; i++)
	{
		addvalidmode (stdres[i][0], stdres[i][1], 8, 1, 0, 0, 0 - 1);
		addvalidmode (stdres[i][0], stdres[i][1], 8, 0, 0, 0, 0 - 1);
	}
	sortvalidmodes ();
}

int setvideomode (int xdim, int ydim, int bitspp, int fullsc)
{
	int i, j, modenum, regrab;
	if (fullsc == fullscreen && xdim == xres && ydim == yres && bitspp == bpp && !videomodereset)
	{
		OSD_ResizeDisplay (xres, yres);
		return 0;
	}
	if (bitspp != 8)
		return 0 - 1;
	modenum = checkvideomode (&xdim, &ydim, bitspp, fullsc, 0);
	if (modenum < 0)
		return 0 - 1;
	regrab = 0;
	if (mouseacquired)
	{
		regrab = 1;
		grabmouse (0);
	}
	if (baselayer_videomodewillchange)
		baselayer_videomodewillchange ();
	if (frame)
		free (frame);
	frame = (unsigned char *)calloc (xdim * ydim, 1);
	if (!frame)
	{
		buildputs ("Unable to allocate framebuffer\n");
		return 0 - 1;
	}
	frameplace = (intptr_t)frame;
	bytesperline = xdim;
	imageSize = xdim * ydim;
	numpages = 1;
	setvlinebpl (bytesperline);
	for (i = j = 0; i <= ydim; i++)
	{
		ylookup[i] = j;
		j += bytesperline;
	}
	sw_plat_mode (xdim, ydim);
	xres = xdim;
	yres = ydim;
	bpp = bitspp;
	fullscreen = fullsc;
	videomodereset = 0;
	if (baselayer_videomodedidchange)
		baselayer_videomodedidchange ();
	OSD_ResizeDisplay (xres, yres);
	setpalettefade (palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);
	if (regrab)
		grabmouse (1);
	return 0;
}

const char *getdisplayname (int display)
{
	return display == 0 ? "C4" : NULL;
}

static void present (void)
{
	if (frame)
		sw_plat_frame (frame, xres, yres, pal32);
}

void showframe (void)
{
	pages++;
	if (frame)
		sw_sum_frame (frame, xres, yres);
	present ();
}

// The palette as faded (curpalettefaded, 0-255 a component). A new one
// is shown at once, with the frame last drawn, as SDL's 8-bit screen
// showed it: a game fades the screen by the palette alone.
int setpalette (int start, int num, unsigned char *dapal)
{
	int i, v, changed;
	(void)start; (void)num; (void)dapal;
	changed = 0;
	for (i = 0; i < 256; i++)
	{
		v = (curpalettefaded[i].r << 16) | (curpalettefaded[i].g << 8) | curpalettefaded[i].b;
		if (pal32[i] != v)
		{
			pal32[i] = v;
			changed = 1;
		}
	}
	sw_sum_palette ((uint8_t *)curpalettefaded);
	if (changed)
		present ();
	return 0;
}

int setsysgamma (float shadergamma, float sysgamma)
{
	(void)shadergamma; (void)sysgamma;
	return 0;
}
