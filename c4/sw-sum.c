// sw-sum.c -- what every build of c4's port shares besides the platform
// layer: the clock, one rand for every build, the GRP, and the test.
//
// -sum starts a run on a clock that moves only as the game runs, and
// -script plays a fixed set of key presses and mouse moves, frame by
// frame, from the moment the level starts. Every frame is checksummed
// with its palette, and gcc's build and c4lc's, on the bare machine, under
// C4DOS and inside C4IX, must show the same frames (Duke Nukem 3D's port
// does the same: games/duke3d/c4/duke-sum.c in c4).
//
//   -sum [N]     the test's clock; a line every N frames, and the sum at the end
//   -frames N    stop after N frames
//   -script      play the test's keys and mouse
//   -menu        or change the screen size in the game's menu
//   -show        and show the frames, so that a run's time counts the drawing
//   -shot N      every Nth frame as shotN.ppm
//   -home        play the person's own SW.GRP, from /home
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "sw-plat.h"

int sw_testing;
int sw_showing;
int sw_home;
static int dosum, stopat, script, shot, menu;
static int frames;
static uint32_t sum, virt_ms, ms0;
static uint8_t pal[1024];

// ---- rand: the same in every build (compiled as -Drand=sw_rand) ----

static uint32_t rand_s = 1;
int sw_rand (void)
{
	rand_s = rand_s * 1103515245u + 12345u;
	return (rand_s >> 16) & 0x7fff;
}

void sw_srand (unsigned s)
{
	rand_s = s;
}

// ---- the clock ----

// A test's clock moves a millisecond each time it is read, which is the
// same in every build, and a frame's time at each frame (a game tic is
// 1/40 s; a frame is 1/30): the game then runs about a tic a frame,
// whatever the machine.
uint32_t sw_ms (void)
{
	if (sw_testing)
		return ++virt_ms;
	return sw_plat_ms ();
}

void sw_sleep (int ms)
{
	if (sw_testing)
		virt_ms += ms;
	else
		sw_plat_sleep (ms);
}

// ---- the command line, before the game reads it ----

static void quitted (void);

void sw_sum_args (int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp (argv[i], "-sum"))
		{
			dosum = i + 1 < argc && argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9' ? atoi (argv[i + 1]) : 1;
			sw_testing = 1;
			atexit (quitted);
		}
		else if (!strcmp (argv[i], "-frames") && i + 1 < argc)
			stopat = atoi (argv[i + 1]);
		else if (!strcmp (argv[i], "-script"))
			script = 1;
		else if (!strcmp (argv[i], "-menu"))
		{
			script = 1;
			menu = 1;
		}
		else if (!strcmp (argv[i], "-show"))
			sw_showing = 1;
		else if (!strcmp (argv[i], "-home"))
			sw_home = 1;
		else if (!strcmp (argv[i], "-shot") && i + 1 < argc)
			shot = atoi (argv[i + 1]);
	}
	if (!sw_testing)
		sw_showing = 1;
	// On C4IX the game lives in /usr/games/sw, and starts there; on the
	// bare machine and under C4DOS there is only the one directory, and the
	// chdir fails.
	chdir ("/usr/games/sw");
}

// ---- the GRP ----

// The shareware episode is SWSW.GRP on c4's disks, so that the person's
// own SW.GRP never hides it. With -home the game plays theirs alone, added
// with the page's Uploads: in the folder they can be put in for the game
// first, then in /home -- on C4IX /home/sw and /home, on the bare machine
// and under C4DOS sw/ at the disk's root, and the root.

static int found (char *name)
{
	FILE *f;
	if (!(f = fopen (name, "rb")))
		return 0;
	fclose (f);
	return 1;
}

char *sw_find_grp (void)
{
	static char *mine[] = { "/home/sw/sw.grp", "/home/sw/SW.GRP", "/home/SW.GRP", "/home/sw.grp", 0 };
	static char *disk[] = { "sw/sw.grp", "sw/SW.GRP", "SW.GRP", "sw.grp", 0 };
	static char *ours[] = { "SWSW.GRP", "swsw.grp", 0 };
	struct stat st;
	char **n;
	for (n = !sw_home ? ours : stat ("/home", &st) == 0 ? mine : disk; *n; n++)
		if (found (*n))
		{
			printf ("sw: playing %s\n", *n);
			return *n;
		}
	if (sw_home)
		printf ("sw -home plays your own SW.GRP, from Shadow Warrior 1.2 (the registered game):\n"
			"add it with the page's Uploads (in /home/sw or /home), then start it again.\n");
	else
		printf ("sw: the shareware episode, SWSW.GRP, is not here\n");
	return 0;
}

// ---- the test's input ----

// From the frame the level is first drawn: [first, last) a key held (a
// browser key code), or the mouse moved each frame by dx, dy, or a button.
enum { S_KEY = 1, S_MOUSE = 2, S_BUTTON = 3 };
static int steps[][5] = {
	{ S_KEY,     20,  90, 38, 0 },	// up: walk
	{ S_KEY,     90, 105, 37, 0 },	// left: turn
	{ S_KEY,    105, 160, 38, 0 },
	{ S_KEY,    140, 150, 17, 0 },	// ctrl: fire
	{ S_KEY,    160, 190, 39, 0 },	// right
	{ S_KEY,    170, 230, 38, 0 },
	{ S_KEY,    230, 236, 32, 0 },	// space: open
	{ S_MOUSE,  240, 270, 12, 0 },	// the mouse turns
	{ S_MOUSE,  270, 280, 0, -6 },	// and looks up
	{ S_BUTTON, 280, 290, 0, 0 },	// and fires
	{ S_KEY,    290, 330, 40, 0 },	// down: back
	{ S_KEY,    300, 302, 50, 0 },	// 2: a weapon key
	{ S_KEY,    330, 400, 38, 0 },
	{ S_KEY,    335, 340, 16, 0 },	// shift: run
	{ S_KEY,    350, 380, 65, 0 },	// a: jump
	{ S_KEY,    380, 400, 90, 0 },	// z: crouch
	{ S_KEY,    400, 460, 37, 0 },
	{ S_KEY,    410, 440, 17, 0 },
	{ 0, 0, 0, 0, 0 }
};
// -menu: the screen size changed in the game, as a player does it; then
// a walk, and a quit from the game, which must end the program. (The
// lines are set once the game's menus have been played through.)
static int menusteps[][5] = {
	{ S_KEY,  20,  22, 27, 0 },	// esc: the menu
	{ S_KEY,  30,  32, 27, 0 },	// esc: out
	{ S_KEY,  40, 100, 38, 0 },	// up: walk
	{ 0, 0, 0, 0, 0 }
};
static int level0 = -1;		// the frame the level was first drawn

void sw_sum_level (void)
{
	if (level0 < 0)
		level0 = frames;
}

void sw_sum_input (void)
{
	int i, f, *p;
	static int last = -1;
	if (!script || level0 < 0)
		return;
	f = frames - level0;
	if (f == last)
		return;
	last = f;
	// five words a step: what, first, last, key or dx, dy
	p = menu ? &menusteps[0][0] : &steps[0][0];
	for (i = 0; p[i * 5]; i++)
	{
		if (p[i * 5 + 0] == S_KEY)
		{
			if (f == p[i * 5 + 1])
				sw_key (p[i * 5 + 3], 1);
			if (f == p[i * 5 + 2])
				sw_key (p[i * 5 + 3], 0);
		}
		else if (p[i * 5 + 0] == S_MOUSE)
		{
			if (f >= p[i * 5 + 1] && f < p[i * 5 + 2])
				sw_mouse_move (p[i * 5 + 3], p[i * 5 + 4]);
		}
		else if (p[i * 5 + 0] == S_BUTTON)
		{
			if (f == p[i * 5 + 1])
				sw_mouse_button (0, 1);
			if (f == p[i * 5 + 2])
				sw_mouse_button (0, 0);
		}
	}
}

// ---- the frames ----

// 256 entries of r, g, b and a spare byte, each 0-255 (curpalettefaded)
void sw_sum_palette (uint8_t *palette)
{
	memcpy (pal, palette, 1024);
}

static int ended;

static void done (void)
{
	uint32_t ms;
	ended = 1;
	ms = sw_plat_ms () - ms0;
	if (!ms)
		ms = 1;
	printf ("speed: %d frames in %d.%03d s, %d.%d fps\n", frames, (int)(ms / 1000), (int)(ms % 1000),
		(int)(frames * 1000 / ms), (int)(frames * 10000 / ms % 10));
	printf ("timedemo: %d frames, checksum %08x\n", frames, sum);
	sw_plat_close ();
	exit (0);
}

// a test's game that ended itself, by its menu's quit (-menu does)
static void quitted (void)
{
	if (ended)
		return;
	ended = 1;
	printf ("timedemo: %d frames, checksum %08x\n", frames, sum);
	printf ("sw: quit from the game\n");
}

void sw_sum_frame (uint8_t *pixels, int w, int h)
{
	uint32_t s, *p;
	int i, n;
	if (!frames)
		ms0 = sw_plat_ms ();
	++frames;
	if (sw_testing)
		virt_ms += 33;
	if (dosum)
	{
		s = sum;
		p = (uint32_t *)pixels;
		n = w * h / 4;
		for (i = 0; i < n; i++)
			s = s * 31 + p[i];
		p = (uint32_t *)pal;
		for (i = 0; i < 256; i++)
			s = s * 31 + (p[i] & 0xffffff);
		s = s * 31 + w * 65536 + h;
		sum = s;
		if (dosum > 1 && frames % dosum == 0)
			printf ("sum: frame %d %08x\n", frames, s);
	}
	// -shot N: frame N as shotN.ppm, to see what a test plays
	if (shot && frames % shot == 0)
	{
		char name[32];
		FILE *f;
		sprintf (name, "shot%d.ppm", frames);
		if ((f = fopen (name, "wb")))
		{
			fprintf (f, "P6\n%d %d\n255\n", w, h);
			for (i = 0; i < w * h; i++)
			{
				fputc (pal[pixels[i] * 4], f);
				fputc (pal[pixels[i] * 4 + 1], f);
				fputc (pal[pixels[i] * 4 + 2], f);
			}
			fclose (f);
		}
	}
	if (stopat && frames >= stopat)
		done ();
}
