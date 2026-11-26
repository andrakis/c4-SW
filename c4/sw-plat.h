// sw-plat.h -- the layers of c4's port under the Build engine (JFBuild):
//
//   sw-layer.c    JFBuild's platform layer (baselayer.h), in place of its
//                 SDL one (sdlayer2.c): the same in every build
//   sw-sum.c      the clock, a test's input and every frame's checksum
//   sw-gui.c      libjs's display, with sw-c4ix.c or sw-bare.c
//   sw-host.c     no display: gcc's build, the reference
//   sw-snd.c      the sound card (sw-nosnd.c: none)
#ifndef C4SW_PLAT_H
#define C4SW_PLAT_H 1

#include <stdint.h>

// events, as libjs's display numbers them (gui.h), and one of its own:
// ev[0] the kind; a key's ev[1] is its browser key code, ev[2] its
// character (0 for none) and ev[3] the modifiers; a button's ev[3] is
// the button (0 left, 1 middle, 2 right), the wheel's its direction
enum {
	SW_EV_DOWN = 2, SW_EV_UP = 3, SW_EV_KEYDOWN = 4, SW_EV_KEYUP = 5,
	SW_EV_WHEEL = 8, SW_EV_REL = 9, SW_EV_QUIT = 100
};

// the platform: sw-gui.c or sw-host.c
void     sw_plat_init (int argc, char **argv);
void     sw_plat_mode (int w, int h);		// the game's screen is now w x h
void     sw_plat_frame (uint8_t *pixels, int w, int h, int *pal);
int      sw_plat_poll (int *ev);			// 1 and ev[0..3], or 0 for none
void     sw_plat_grab (int on);			// the mouse locked to the display
void     sw_plat_close (void);
uint32_t sw_plat_ms (void);
void     sw_plat_sleep (int ms);

// sw-sum.c
extern int sw_testing;				// -sum: a test, on its own clock
extern int sw_showing;				// the frames go to the display
extern int sw_home;				// -home: the person's own GRP
void     sw_sum_args (int argc, char **argv);
void     sw_sum_level (void);			// a level is being played
uint32_t sw_ms (void);
void     sw_sleep (int ms);
void     sw_sum_frame (uint8_t *pixels, int w, int h);
void     sw_sum_palette (uint8_t *palette);	// 256 entries of r, g, b, 0 (0-255)
void     sw_sum_input (void);
char    *sw_find_grp (void);			// the GRP to play, or 0

// sw-snd.c (or sw-nosnd.c): sounds let go, and the music run ahead
void     sw_snd_poll (void);

// sw-layer.c
void     sw_key (int code, int down);		// a browser key code
void     sw_mouse_button (int button, int down);	// 0 left, 1 middle, 2 right
void     sw_mouse_wheel (int up);
void     sw_mouse_move (int dx, int dy);
void     sw_quit (void);

#endif
