// sw-host.c -- the port's platform on a host, built with gcc: no display
// and no sound. The reference the c4lc builds are held to: a test's frames
// are checksummed (sw-sum.c) and nothing is shown.
#include <unistd.h>
#include <sys/time.h>
#include "sw-plat.h"

void sw_plat_init (int argc, char **argv) {}
void sw_plat_mode (int w, int h) {}
void sw_plat_frame (uint8_t *pixels, int w, int h, int *pal) {}
int  sw_plat_poll (int *ev) { return 0; }
void sw_plat_grab (int on) {}
void sw_plat_close (void) {}

uint32_t sw_plat_ms (void)
{
	struct timeval tv;
	gettimeofday (&tv, 0);
	return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void sw_plat_sleep (int ms)
{
	if (ms > 0)
		usleep (ms * 1000);
}
