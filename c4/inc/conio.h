// conio.h -- DOS's port I/O, as audiolib's AdLib driver (al_midi.c) uses
// it: the OPL's address port and data port, 0x388 and 0x389, which
// sw-snd.c turns into register writes on libjs's sound card. Reads are
// the status the delay loops spin on, and mean nothing here.
#ifndef C4SW_CONIO_H
#define C4SW_CONIO_H 1
void sw_outp (int port, int value);
#define outp(port, value) sw_outp ((port), (value))
#define inp(port) 0
#endif
