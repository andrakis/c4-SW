// sw-snd.c -- the game's sound, jfaudiolib's FX_ (effects), MUSIC_ (MIDI)
// and CD_ interfaces, on libjs's sound card (libjs/guest/snd.h), in place
// of jfaudiolib itself. Taken from c4's Duke Nukem 3D port (games/duke3d,
// c4/duke-snd.c), which plays Apogee's audiolib on the same card, as Rise
// of the Triad's does.
//
// The effects are the GRP's VOC and WAV sounds. They play as they are on
// the card's sixteen sample channels, at their own rate moved by the pitch
// the game asks for, panned by angle and distance as audiolib's mixer
// panned them. With all sixteen playing, a new sound takes the oldest of
// the lowest priority (one panned out of hearing first), as audiolib did.
//
// The music is the DOS game's own: audiolib's MIDI player (audiolib/midi.c)
// and its AdLib driver (audiolib/al_midi.c) with the game's own timbre bank
// (SWTIMBR.TMB, which the game registers). The card's OPL stands where the
// AdLib was: al_midi.c's port writes come to sw_outp here. audiolib's task
// manager ran the player from the timer interrupt; here it is sw_snd_poll(),
// which the platform layer calls whenever the game reads its input or the
// clock. The poll runs the player a tick at a time, with snd_wait between
// ticks, so each write is stamped with the stream's time, until the card's
// stream is a quarter of a second ahead.
//
// When the card has a General MIDI synth on its MIDI port (the person
// chose General MIDI music), the player's events go there instead, and
// midi.c plays the score's General MIDI tracks.
//
// There is no CD: the registered game's CD music (and its Ogg copies)
// is not played. With no card, or no OPL, the game plays silent rather
// than stopping, as it would on a DOS machine that had none.

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fx_man.h"
#include "music.h"
#include "cd.h"
#include "sndcards.h"
#ifndef cdecl
#define cdecl			// Watcom's calling convention, in audiolib's headers
#endif
#include "audiolib/midi.h"
#include "audiolib/al_midi.h"
#include "audiolib/task_man.h"
#include "snd.h"
#include "sw-plat.h"

// a quarter of a second ahead, as Doom keeps its music: a frame that
// takes longer (a level's first) then costs no gap
enum { AHEAD_US = 250000, MAXSTEPS = 4000 };

static int fx_on, music_on;
int MUSIC_SoundDevice;                    // midi.c's, which asks it nothing here
int FX_ErrorCode, MUSIC_ErrorCode;
enum { C4_NoCard = 100 };                 // this file's own error: no card

static void no_messages(const char *s) { (void) s; }
void (*ASS_MessageOutputString)(const char *) = no_messages;

// ---- the task manager, run by the poll ----

enum { NTASKS = 4 };
static task tasks[NTASKS];
static int  task_used[NTASKS];
static long task_next[NTASKS];    // when each runs next, on the stream's clock
static long stream_us;            // the stream's clock: what snd_wait has moved
volatile int TS_InInterrupt = 0;

task *TS_ScheduleTask(void (*function)(task *), int rate, int priority, void *data) {
	int i;
	for (i = 0; i < NTASKS; i++)
		if (!task_used[i]) {
			task_used[i] = 1;
			tasks[i].TaskService = function;
			tasks[i].rate = rate;
			tasks[i].priority = priority;
			tasks[i].data = data;
			tasks[i].count = 0;
			tasks[i].active = 0;
			task_next[i] = stream_us;
			return &tasks[i];
		}
	return NULL;
}
int TS_Terminate(task *t) {
	int i;
	for (i = 0; i < NTASKS; i++) if (&tasks[i] == t) task_used[i] = 0;
	return TASK_Ok;
}
void TS_Dispatch(void) {
	int i;
	for (i = 0; i < NTASKS; i++) if (task_used[i]) tasks[i].active = 1;
}
void TS_SetTaskRate(task *t, int rate) { t->rate = rate; }
void TS_Shutdown(void) {
	int i;
	for (i = 0; i < NTASKS; i++) task_used[i] = 0;
}
int  TS_LockMemory(void) { return TASK_Ok; }
void TS_UnlockMemory(void) {}

int USER_CheckParameter(const char *parameter) { (void) parameter; return 0; }

// What audiolib asked of DOS: memory locked against paging, and interrupts
// off around what its timer task touched. The task runs from the poll
// here, never in the middle of anything, so none of it is needed.
int DPMI_LockMemory(void *address, unsigned length) { (void) address; (void) length; return 0; }
int DPMI_LockMemoryRegion(void *start, void *end) { (void) start; (void) end; return 0; }
int DPMI_UnlockMemory(void *address, unsigned length) { (void) address; (void) length; return 0; }
int DPMI_UnlockMemoryRegion(void *start, void *end) { (void) start; (void) end; return 0; }
unsigned long DisableInterrupts(void) { return 0; }
void RestoreInterrupts(unsigned long flags) { (void) flags; }

// General MIDI: midi.c's events as messages on the card's stream, as they
// went to an MPU-401, and the score's General MIDI tracks (EMIDI)
int c4_music_gm;                         // midi.c's too: it picks the General MIDI tracks
static void gm_noteoff(int ch, int key, int vel) { snd_midi(0x80 | ch, key, vel); }
static void gm_noteon(int ch, int key, int vel) { snd_midi(0x90 | ch, key, vel); }
static void gm_polyaftertouch(int ch, int key, int pressure) { snd_midi(0xa0 | ch, key, pressure); }
static void gm_controlchange(int ch, int number, int value) { snd_midi(0xb0 | ch, number, value); }
static void gm_programchange(int ch, int program) { snd_midi(0xc0 | ch, program, 0); }
static void gm_channelaftertouch(int ch, int pressure) { snd_midi(0xd0 | ch, pressure, 0); }
static void gm_pitchbend(int ch, int lsb, int msb) { snd_midi(0xe0 | ch, lsb, msb); }
static char gm_on[6] = { (char) 0xf0, 0x7e, 0x7f, 0x09, 0x01, (char) 0xf7 };   // General MIDI System On

// al_midi.c's AdLib, at 0x388 (register) and 0x389 (data): the card's OPL
static int opl_reg;
void sw_outp(int port, int value) {
	if ((port & 1) == 0) opl_reg = value & 255;
	else snd_opl(opl_reg, value & 255);
}

// ---- the effects ----

enum { NVOICES = SND_CHANNELS };
typedef struct {
	int handle;              // 0: free
	int priority;
	unsigned int callbackval;
	int loop;
	char *data;              // the samples, unsigned 8-bit mono
	int len, rate, vol;
	char *own;               // converted samples this voice allocated, or 0
	unsigned started;        // when it started, in sounds started: the oldest is the least
	int paused;
} fxvoice;
static fxvoice voice[NVOICES];
static int next_handle = 1;
static unsigned fx_started;
static int fx_volume = 255, fx_reverse;
static void (*fx_callback)(unsigned int);

static void fx_end(int i) {
	fxvoice *v = &voice[i];
	unsigned int cb = v->callbackval;
	int was = v->handle;
	if (v->own) free(v->own);
	v->own = 0;
	v->handle = 0;
	if (was && fx_callback) fx_callback(cb);
}

// voices whose channel has gone quiet: looped ones play again, the rest
// are finished, and the game is told
static void fx_reap(void) {
	int i, busy = snd_busy();
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle && !voice[i].paused && !(busy & (1 << i))) {
			if (voice[i].loop) snd_play(i, voice[i].data, voice[i].len, voice[i].rate, voice[i].vol);
			else fx_end(i);
		}
}

// a voice playing at nothing on either side: a sound panned out of
// hearing, which losing costs nothing
static int fx_silent(int i) { return voice[i].vol == 0; }

// a channel for a sound of this priority: a free one; else, as audiolib's
// MV_AllocVoice chose, one of the lowest priority playing, if that is at
// most this sound's -- of those, one that cannot be heard, else the one
// that started first. -1 if there is none.
static int fx_channel(int priority) {
	int i, low = -1;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (!voice[i].handle) return i;
	for (i = 0; i < NVOICES; i++) {
		if (voice[i].priority > priority) continue;
		if (low < 0 || voice[i].priority < voice[low].priority) { low = i; continue; }
		if (voice[i].priority > voice[low].priority || fx_silent(low)) continue;
		if (fx_silent(i) || (int) (voice[i].started - voice[low].started) < 0) low = i;
	}
	return low;
}

static int fx_find(int handle) {
	int i;
	if (handle <= 0) return -1;
	for (i = 0; i < NVOICES; i++) if (voice[i].handle == handle) return i;
	return -1;
}

// left and right (0-255 each) as the card wants them, the volume and the
// reversal applied
static int fx_lr(int left, int right) {
	int t;
	if (left < 0) left = 0;
	if (left > 255) left = 255;
	if (right < 0) right = 0;
	if (right > 255) right = 255;
	if (fx_reverse) { t = left; left = right; right = t; }
	left = left * fx_volume / 255;
	right = right * fx_volume / 255;
	return left | (right << 8);
}

// the rate moved by PITCHOFFSET cents, as audiolib's PITCH_GetScale did
static int fx_pitched(int rate, int cents) {
	static int semitone[13] = {
		65536, 69433, 73562, 77936, 82570, 87480, 92682,
		98193, 104032, 110218, 116773, 123716, 131072 };
	int oct, n, s, scale;
	if (cents == 0) return rate;
	oct = cents / 1200;
	n = cents - oct * 1200;
	if (n < 0) { n += 1200; oct--; }
	s = n / 100;
	scale = semitone[s] + (semitone[s + 1] - semitone[s]) * (n % 100) / 100;
	// rate * scale >> 16 in 32 bits: the rate's high and low halves
	rate = (rate >> 16) * scale + (((rate & 0xffff) * (scale >> 4)) >> 12);
	if (oct > 0) rate <<= oct;
	if (oct < 0) rate >>= -oct;
	return rate;
}

// audiolib's MV_Pan3D and MV_CalcPanTable: 32 angles round the listener,
// 0 ahead and 8 to the right; the far ear's level falls to nothing over a
// quarter turn, and distance (0-255) takes the level down for both
static void fx_pan3d(int angle, int distance, int *left, int *right) {
	int level, i, d;
	if (distance < 0) { distance = -distance; angle += 16; }
	if (distance > 255) distance = 255;
	level = 255 - distance;
	i = angle & 31;
	*left = *right = level;
	if (i <= 16) {
		d = i <= 8 ? i : 16 - i;
		*left = level - level * d / 8;
	} else {
		d = i <= 22 ? i - 16 : i <= 24 ? 8 : 31 - i;
		*right = level - level * d / 8;
	}
}

static int le16(unsigned char *p) { return p[0] | (p[1] << 8); }
static int le32(unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

// A Creative Voice File: its sound data, (type 1) blocks with their
// continuations (type 2), or type 9 blocks of 8-bit unsigned or 16-bit
// signed PCM, mono or stereo, made 8-bit mono. *own is set when the
// samples had to go into memory of their own. As Duke Nukem 3D's port.
static int voc_parse(unsigned char *p, unsigned int plen, char **data, int *len, int *rate, char **own) {
	unsigned char *q, *body, *end;
	char *join;
	int type, blen, n, total, codec, bits = 8, channels = 1, i, frames, v;
	*own = 0;
	if (plen < 26 || memcmp(p, "Creative Voice File", 19) != 0) return 0;
	end = p + plen;
	q = p + le16(p + 20);
	total = 0; n = 0; *rate = 0; *data = 0;
	// first pass: how much there is, and where the first block's samples are
	for (body = q; body + 4 <= end && (type = body[0]) != 0; body = body + 4 + blen) {
		blen = body[1] | (body[2] << 8) | (body[3] << 16);
		if (body + 4 + blen > end) blen = (int) (end - body) - 4;
		if (type == 1) {
			codec = body[5];
			if (codec != 0) return 0;              // only unsigned 8-bit PCM
			if (!*rate) *rate = 1000000 / (256 - body[4]);
			if (!n) *data = (char *) body + 6;
			total += blen - 2; n++;
		} else if (type == 2) {
			total += blen; n++;
		} else if (type == 9) {
			if (!((body[8] == 8 && le16(body + 10) == 0) || (body[8] == 16 && le16(body + 10) == 4)) ||
			    (body[9] != 1 && body[9] != 2)) return 0;
			if (n && (body[8] != bits || body[9] != channels)) return 0;
			bits = body[8]; channels = body[9];
			*rate = le32(body + 4);
			if (!n) *data = (char *) body + 16;
			total += blen - 12; n++;
		} else if (type == 7) break;
	}
	if (!n || !*rate || total <= 0) return 0;
	*len = total;
	if (n > 1 || bits != 8 || channels != 1) {
		if (!(join = malloc(total))) return 0;
		total = 0;
		for (body = q; body + 4 <= end && (type = body[0]) != 0 && type != 7; body = body + 4 + blen) {
			blen = body[1] | (body[2] << 8) | (body[3] << 16);
			if (body + 4 + blen > end) blen = (int) (end - body) - 4;
			if (type == 1) { memcpy(join + total, body + 6, blen - 2); total += blen - 2; }
			else if (type == 2) { memcpy(join + total, body + 4, blen); total += blen; }
			else if (type == 9) { memcpy(join + total, body + 16, blen - 12); total += blen - 12; }
		}
		*data = join; *own = join;
	}
	if (bits == 8 && channels == 1) return 1;
	// 16-bit or stereo: 8-bit mono, the channels' mean, in place in the
	// memory already its own
	frames = *len / (channels * (bits / 8));
	q = (unsigned char *) *own;
	for (i = 0; i < frames; i++) {
		if (bits == 8) v = (q[i * channels] + q[i * channels + channels - 1]) / 2;
		else {
			v = (short) le16(q + i * channels * 2) + (short) le16(q + (i * channels + channels - 1) * 2);
			v = (v / 2 >> 8) + 128;
		}
		(*own)[i] = (char) v;
	}
	*len = frames;
	return 1;
}

// A RIFF WAVE: 8-bit as it is; 16-bit, or stereo, made 8-bit mono in
// memory of its own
static int wav_parse(unsigned char *p, unsigned int plen, char **data, int *len, int *rate, char **own) {
	unsigned char *q, *end, *pcm = 0;
	int n = 0, size;
	int channels = 0, bits = 0, i, frames, v;
	char *out;
	*own = 0;
	if (plen < 12 || memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0) return 0;
	end = p + 8 + le32(p + 4);
	if (end > p + plen) end = p + plen;
	for (q = p + 12; q + 8 <= end; q = q + 8 + ((size + 1) & ~1)) {
		size = le32(q + 4);
		if (memcmp(q, "fmt ", 4) == 0) {
			if (le16(q + 8) != 1) return 0;      // PCM
			channels = le16(q + 10);
			*rate = le32(q + 12);
			bits = le16(q + 22);
		} else if (memcmp(q, "data", 4) == 0) {
			pcm = q + 8; n = size;
			if (pcm + n > end) n = (int) (end - pcm);
		}
	}
	if (!pcm || !channels || (bits != 8 && bits != 16)) return 0;
	if (bits == 8 && channels == 1) { *data = (char *) pcm; *len = n; return 1; }
	frames = n / (channels * (bits / 8));
	if (!(out = malloc(frames))) return 0;
	for (i = 0; i < frames; i++) {
		if (bits == 8) v = (pcm[i * channels] + pcm[i * channels + channels - 1]) / 2;
		else {
			v = (short) le16(pcm + i * channels * 2) + (short) le16(pcm + (i * channels + channels - 1) * 2);
			v = (v / 2 >> 8) + 128;
		}
		out[i] = (char) v;
	}
	*data = out; *len = frames; *own = out;
	return 1;
}

static int fx_start(char *data, int len, int rate, char *own, int pitchoffset, int left, int right,
                    int priority, unsigned int callbackval, int loop) {
	int i;
	fxvoice *v;
	if (!fx_on || len <= 0) { if (own) free(own); return FX_Warning; }
	if ((i = fx_channel(priority)) < 0) { if (own) free(own); return FX_Warning; }
	if (voice[i].handle) { snd_stop(i); fx_end(i); }
	v = &voice[i];
	v->handle = next_handle++;
	if (next_handle > 0x7fff) next_handle = 1;
	v->priority = priority;
	v->started = fx_started++;
	v->callbackval = callbackval;
	v->loop = loop;
	v->paused = 0;
	v->data = data; v->len = len; v->own = own;
	v->rate = fx_pitched(rate, pitchoffset);
	v->vol = fx_lr(left, right);
	snd_play(i, v->data, v->len, v->rate, v->vol);
	return v->handle;
}

static int fx_play(char *ptr, unsigned int plen, int pitchoffset, int left, int right, int priority,
                   unsigned int callbackval, int loop) {
	char *data, *own;
	int len, rate;
	if (!fx_on) return FX_Warning;
	if (!voc_parse((unsigned char *) ptr, plen, &data, &len, &rate, &own) &&
	    !wav_parse((unsigned char *) ptr, plen, &data, &len, &rate, &own)) {
		FX_ErrorCode = FX_InvalidCard;
		return FX_Error;
	}
	return fx_start(data, len, rate, own, pitchoffset, left, right, priority, callbackval, loop);
}

static int has_card(int what) { return snd_present() && (snd_caps() & what); }

const char *FX_ErrorString(int ErrorNumber) {
	if (ErrorNumber == FX_Warning || ErrorNumber == FX_Error) ErrorNumber = FX_ErrorCode;
	if (ErrorNumber == FX_Ok) return "Fx ok.";
	if (ErrorNumber == C4_NoCard) return "The machine has no sound card with sample channels.";
	return "Unknown Fx error code.";
}
int FX_Init(int SoundCard, int numvoices, int *numchannels, int *samplebits, int *mixrate, void *initdata) {
	(void) SoundCard; (void) numvoices; (void) initdata;
	*numchannels = 2;
	*samplebits = 8;
	(void) mixrate;
	if (fx_on) FX_Shutdown();
	// no card: the game plays silent rather than without its menus' sounds
	if (!has_card(SND_HAS_PCM)) { FX_ErrorCode = C4_NoCard; return FX_Ok; }
	memset(voice, 0, sizeof voice);
	fx_on = 1;
	return FX_Ok;
}
int FX_Shutdown(void) {
	if (fx_on) FX_StopAllSounds();
	fx_on = 0;
	return FX_Ok;
}
int FX_GetCurrentDriver(void) { return fx_on ? ASS_SDL : ASS_NoSound; }
const char *FX_GetCurrentDriverName(void) { return fx_on ? "c4's sound card" : "none"; }
int  FX_SetCallBack(void (*function)(unsigned int)) { fx_callback = function; return FX_Ok; }
void FX_SetVolume(int volume) { fx_volume = volume < 0 ? 0 : volume > 255 ? 255 : volume; }
int  FX_GetVolume(void) { return fx_volume; }
void FX_SetReverseStereo(int setting) { fx_reverse = setting; }
int  FX_GetReverseStereo(void) { return fx_reverse; }
void FX_SetReverb(int reverb) { (void) reverb; }
void FX_SetFastReverb(int reverb) { (void) reverb; }
int  FX_GetMaxReverbDelay(void) { return 0; }
int  FX_GetReverbDelay(void) { return 0; }
void FX_SetReverbDelay(int delay) { (void) delay; }
int  FX_VoiceAvailable(int priority) {
	int i;
	if (!fx_on) return 0;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (!voice[i].handle || voice[i].priority <= priority) return 1;
	return 0;
}
int  FX_EndLooping(int handle) {
	int i = fx_find(handle);
	if (i >= 0) voice[i].loop = 0;
	return FX_Ok;
}
int  FX_SetPan(int handle, int vol, int left, int right) {
	int i = fx_find(handle);
	(void) vol;
	if (i < 0) return FX_Warning;
	voice[i].vol = fx_lr(left, right);
	snd_volume(i, voice[i].vol);
	return FX_Ok;
}
// a sound's pitch is its rate, and the card takes that when it starts
int  FX_SetPitch(int handle, int pitchoffset) { (void) handle; (void) pitchoffset; return FX_Ok; }
int  FX_SetFrequency(int handle, int frequency) { (void) handle; (void) frequency; return FX_Ok; }
int  FX_GetFrequency(int handle, int *frequency) {
	int i = fx_find(handle);
	*frequency = i >= 0 ? voice[i].rate : 0;
	return i >= 0 ? FX_Ok : FX_Warning;
}
int FX_PlayVOC(char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
               int priority, unsigned int callbackval) {
	(void) vol;
	return fx_play(ptr, ptrlength, pitchoffset, left, right, priority, callbackval, 0);
}
int FX_PlayLoopedVOC(char *ptr, unsigned int ptrlength, int loopstart, int loopend, int pitchoffset,
                     int vol, int left, int right, int priority, unsigned int callbackval) {
	(void) loopstart; (void) loopend; (void) vol;
	return fx_play(ptr, ptrlength, pitchoffset, left, right, priority, callbackval, 1);
}
int FX_PlayWAV(char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
               int priority, unsigned int callbackval) {
	return FX_PlayVOC(ptr, ptrlength, pitchoffset, vol, left, right, priority, callbackval);
}
int FX_PlayLoopedWAV(char *ptr, unsigned int ptrlength, int loopstart, int loopend, int pitchoffset,
                     int vol, int left, int right, int priority, unsigned int callbackval) {
	return FX_PlayLoopedVOC(ptr, ptrlength, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
}
int FX_PlayVOC3D(char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
                 int priority, unsigned int callbackval) {
	int left, right;
	fx_pan3d(angle, distance, &left, &right);
	return fx_play(ptr, ptrlength, pitchoffset, left, right, priority, callbackval, 0);
}
int FX_PlayWAV3D(char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
                 int priority, unsigned int callbackval) {
	return FX_PlayVOC3D(ptr, ptrlength, pitchoffset, angle, distance, priority, callbackval);
}
// jfaudiolib's "auto": whatever the sound is, by its header
int FX_PlayAuto(char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
                int priority, unsigned int callbackval) {
	return FX_PlayVOC(ptr, ptrlength, pitchoffset, vol, left, right, priority, callbackval);
}
int FX_PlayLoopedAuto(char *ptr, unsigned int ptrlength, int loopstart, int loopend, int pitchoffset,
                      int vol, int left, int right, int priority, unsigned int callbackval) {
	return FX_PlayLoopedVOC(ptr, ptrlength, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
}
int FX_PlayAuto3D(char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
                  int priority, unsigned int callbackval) {
	return FX_PlayVOC3D(ptr, ptrlength, pitchoffset, angle, distance, priority, callbackval);
}
int FX_PlayRaw(char *ptr, unsigned int length, unsigned rate, int pitchoffset, int vol, int left,
               int right, int priority, unsigned int callbackval) {
	(void) vol;
	return fx_start(ptr, (int) length, (int) rate, 0, pitchoffset, left, right, priority, callbackval, 0);
}
int FX_Pan3D(int handle, int angle, int distance) {
	int i = fx_find(handle), left, right;
	if (i < 0) return FX_Warning;
	fx_pan3d(angle, distance, &left, &right);
	voice[i].vol = fx_lr(left, right);
	snd_volume(i, voice[i].vol);
	return FX_Ok;
}
int FX_SoundActive(int handle) {
	int i = fx_find(handle);
	return i >= 0 && ((snd_busy() & (1 << i)) || voice[i].loop || voice[i].paused);
}
int FX_SoundsPlaying(void) {
	int i, n = 0;
	if (!fx_on) return 0;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (voice[i].handle) n++;
	return n;
}
int FX_StopSound(int handle) {
	int i = fx_find(handle);
	if (i < 0) return FX_Warning;
	snd_stop(i);
	fx_end(i);
	return FX_Ok;
}
// the card cannot hold a sound where it was: a paused one is silenced, and
// starts again from its beginning when it goes on
int FX_PauseSound(int handle, int pauseon) {
	int i = fx_find(handle);
	if (i < 0) return FX_Warning;
	if (pauseon && !voice[i].paused) { snd_stop(i); voice[i].paused = 1; }
	else if (!pauseon && voice[i].paused) {
		voice[i].paused = 0;
		snd_play(i, voice[i].data, voice[i].len, voice[i].rate, voice[i].vol);
	}
	return FX_Ok;
}
int FX_StopAllSounds(void) {
	int i;
	if (!fx_on) return FX_Ok;
	snd_stop(-1);
	for (i = 0; i < NVOICES; i++) if (voice[i].handle) fx_end(i);
	return FX_Ok;
}

// ---- the music: audiolib's MIDI player, on the card's OPL ----

static midifuncs al_funcs;

const char *MUSIC_ErrorString(int ErrorNumber) {
	if (ErrorNumber == MUSIC_Warning || ErrorNumber == MUSIC_Error) ErrorNumber = MUSIC_ErrorCode;
	if (ErrorNumber == MUSIC_Ok) return "Music ok.";
	if (ErrorNumber == C4_NoCard) return "The machine has no sound card with an OPL or a MIDI synth.";
	if (ErrorNumber == MUSIC_MidiError) return "Error playing MIDI file.";
	return "Unknown Music error code.";
}
int MUSIC_Init(int SoundCard, const char *params) {
	int i;
	(void) SoundCard; (void) params;
	c4_music_gm = has_card(SND_HAS_MIDI);
	if (!c4_music_gm && !has_card(SND_HAS_OPL)) { MUSIC_ErrorCode = C4_NoCard; return MUSIC_Error; }
	snd_clock(1000000);                     // snd_wait in microseconds
	for (i = 0; i < 128; i++) MIDI_PatchMap[i] = i;
	memset(&al_funcs, 0, sizeof al_funcs);
	if (c4_music_gm) {
		snd_midi_bytes(gm_on, sizeof gm_on);
		al_funcs.NoteOff = gm_noteoff;
		al_funcs.NoteOn = gm_noteon;
		al_funcs.PolyAftertouch = gm_polyaftertouch;
		al_funcs.ControlChange = gm_controlchange;
		al_funcs.ProgramChange = gm_programchange;
		al_funcs.ChannelAftertouch = gm_channelaftertouch;
		al_funcs.PitchBend = gm_pitchbend;
		MIDI_SetMidiFuncs(&al_funcs);
		music_on = 1;
		return MUSIC_Ok;
	}
	// al_midi.c drives an OPL2, and a program before this one may have
	// left the card's OPL3 in its OPL3 mode, where al_midi.c's notes are
	// silent (Duke Nukem 3D's port found Doom does): the OPL2 it knows
	snd_opl(0x104, 0);                      // no four-operator channels
	snd_opl(0x105, 0);
	AL_Init(0);                             // an AdLib whatever the card (al_midi.c)
	al_funcs.NoteOff = AL_NoteOff;
	al_funcs.NoteOn = AL_NoteOn;
	al_funcs.ControlChange = AL_ControlChange;
	al_funcs.ProgramChange = AL_ProgramChange;
	al_funcs.PitchBend = AL_SetPitchBend;
	MIDI_SetMidiFuncs(&al_funcs);
	music_on = 1;
	return MUSIC_Ok;
}
int MUSIC_Shutdown(void) {
	if (!music_on) return MUSIC_Ok;
	MIDI_StopSong();
	if (!c4_music_gm) AL_Shutdown();
	music_on = 0;
	return MUSIC_Ok;
}
int  MUSIC_GetCurrentDriver(void) { return music_on ? ASS_SDL : ASS_NoSound; }
const char *MUSIC_GetCurrentDriverName(void) {
	return !music_on ? "none" : c4_music_gm ? "c4's sound card, General MIDI" : "c4's sound card, OPL";
}
void MUSIC_SetMaxFMMidiChannel(int channel) { if (music_on && !c4_music_gm) AL_SetMaxMidiChannel(channel); }
void MUSIC_SetVolume(int volume) {
	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	if (music_on) MIDI_SetVolume(volume);
}
void MUSIC_SetMidiChannelVolume(int channel, int volume) { if (music_on) MIDI_SetUserChannelVolume(channel, volume); }
void MUSIC_ResetMidiChannelVolumes(void) { if (music_on) MIDI_ResetUserChannelVolume(); }
int  MUSIC_GetVolume(void) { return music_on ? MIDI_GetVolume() : 0; }
void MUSIC_SetLoopFlag(int loopflag) { if (music_on) MIDI_SetLoopFlag(loopflag); }
int  MUSIC_SongPlaying(void) { return music_on ? MIDI_SongPlaying() : 0; }
void MUSIC_Continue(void) { if (music_on) MIDI_ContinueSong(); }
void MUSIC_Pause(void) { if (music_on) MIDI_PauseSong(); }
int  MUSIC_StopSong(void) {
	if (music_on) MIDI_StopSong();
	return MUSIC_Ok;
}
// the song is the game's (LoadSong), kept by it while it plays
int  MUSIC_PlaySong(void *song, unsigned int length, int loopflag) {
	(void) length;
	if (!music_on) return MUSIC_Warning;
	MIDI_StopSong();
	if (MIDI_PlaySong((unsigned char *) song, loopflag) != MIDI_Ok) { MUSIC_ErrorCode = MUSIC_MidiError; return MUSIC_Warning; }
	return MUSIC_Ok;
}
void MUSIC_SetContext(int context) { if (music_on) MIDI_SetContext(context); }
int  MUSIC_GetContext(void) { return music_on ? MIDI_GetContext() : 0; }
void MUSIC_RegisterTimbreBank(unsigned char *timbres) { AL_RegisterTimbreBank(timbres); }

// ---- no CD ----

const char *CD_ErrorString(int code) { (void) code; return "This machine has no CD drive for music."; }
int  CD_GetError(void) { return CD_Error; }
int  CD_Init(int SoundCard) { (void) SoundCard; return CD_Error; }
int  CD_GetCurrentDriver(void) { return ASS_NoSound; }
const char *CD_GetCurrentDriverName(void) { return "none"; }
void CD_Shutdown(void) {}
int  CD_Play(int track, int loop) { (void) track; (void) loop; return CD_Error; }
void CD_Stop(void) {}
void CD_Pause(int pauseon) { (void) pauseon; }
int  CD_IsPlaying(void) { return 0; }
void CD_SetVolume(int volume) { (void) volume; }

// ---- the poll ----

// Called whenever the game reads the clock or its input (sw-layer.c):
// sounds that have finished are let go, and the music's tasks run in
// stream time until the card's stream is AHEAD_US ahead of now. A stream
// that fell behind (a level loading) the card brings up to now, so the
// music pauses rather than catching up.
void sw_snd_poll(void) {
	static int polling;
	int i, k, n;
	if (polling || (!fx_on && !music_on)) return;
	polling = 1;
	if (fx_on) fx_reap();
	// the stream's clock is a 32-bit count of microseconds: moved back,
	// with the tasks' times, long before it could wrap (36 minutes)
	if (stream_us > 1000000000) {
		for (i = 0; i < NTASKS; i++) task_next[i] -= 1000000000;
		stream_us -= 1000000000;
	}
	for (n = 0; music_on && n < MAXSTEPS && snd_ahead() < AHEAD_US; n++) {
		k = -1;
		for (i = 0; i < NTASKS; i++)
			if (task_used[i] && tasks[i].active && tasks[i].rate > 0 && (k < 0 || task_next[i] < task_next[k])) k = i;
		if (k < 0) break;
		if (task_next[k] > stream_us) {
			snd_wait((int) (task_next[k] - stream_us));
			stream_us = task_next[k];
		}
		task_next[k] += 1000000 / tasks[k].rate;
		tasks[k].TaskService(&tasks[k]);
	}
	polling = 0;
}
