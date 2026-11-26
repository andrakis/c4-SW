// sw-nosnd.c -- no sound: jfaudiolib's FX_, MUSIC_ and CD_ calls as the
// game makes them, doing nothing. gcc's build (the reference, which has no
// card) links this in place of sw-snd.c, as Duke Nukem 3D's duke-nosnd.c.
// No sound changes no frame: a test runs with the sound off either way.
#include <stddef.h>
#include "fx_man.h"
#include "music.h"
#include "cd.h"
#include "sw-plat.h"

void sw_snd_poll (void) {}

int FX_ErrorCode, MUSIC_ErrorCode;
static void no_messages (const char *s) { (void)s; }
void (*ASS_MessageOutputString) (const char *) = no_messages;

const char *FX_ErrorString (int e) { (void)e; return "no sound"; }
int FX_Init (int card, int voices, int *channels, int *bits, int *rate, void *init)
{
	(void)card; (void)voices; (void)channels; (void)bits; (void)rate; (void)init;
	return FX_Error;
}
int FX_Shutdown (void) { return FX_Ok; }
int FX_GetCurrentDriver (void) { return 0; }
const char *FX_GetCurrentDriverName (void) { return "none"; }
int FX_SetCallBack (void (*function) (unsigned int)) { (void)function; return FX_Ok; }
void FX_SetVolume (int volume) { (void)volume; }
int FX_GetVolume (void) { return 0; }
void FX_SetReverseStereo (int setting) { (void)setting; }
int FX_GetReverseStereo (void) { return 0; }
void FX_SetReverb (int reverb) { (void)reverb; }
int FX_SetPan (int handle, int vol, int left, int right) { (void)handle; (void)vol; (void)left; (void)right; return FX_Ok; }
int FX_SetPitch (int handle, int pitchoffset) { (void)handle; (void)pitchoffset; return FX_Ok; }
int FX_PlayVOC (char *ptr, unsigned int len, int pitch, int vol, int left, int right, int priority, unsigned int cb)
{
	(void)ptr; (void)len; (void)pitch; (void)vol; (void)left; (void)right; (void)priority; (void)cb;
	return FX_Warning;
}
int FX_PlayAuto (char *ptr, unsigned int len, int pitch, int vol, int left, int right, int priority, unsigned int cb)
{
	(void)ptr; (void)len; (void)pitch; (void)vol; (void)left; (void)right; (void)priority; (void)cb;
	return FX_Warning;
}
int FX_PlayLoopedAuto (char *ptr, unsigned int len, int loopstart, int loopend, int pitch, int vol,
		       int left, int right, int priority, unsigned int cb)
{
	(void)ptr; (void)len; (void)loopstart; (void)loopend; (void)pitch; (void)vol; (void)left; (void)right;
	(void)priority; (void)cb;
	return FX_Warning;
}
int FX_PlayAuto3D (char *ptr, unsigned int len, int pitch, int angle, int distance, int priority, unsigned int cb)
{
	(void)ptr; (void)len; (void)pitch; (void)angle; (void)distance; (void)priority; (void)cb;
	return FX_Warning;
}
int FX_Pan3D (int handle, int angle, int distance) { (void)handle; (void)angle; (void)distance; return FX_Ok; }
int FX_SoundActive (int handle) { (void)handle; return 0; }
int FX_SoundsPlaying (void) { return 0; }
int FX_StopSound (int handle) { (void)handle; return FX_Ok; }
int FX_PauseSound (int handle, int pauseon) { (void)handle; (void)pauseon; return FX_Ok; }
int FX_StopAllSounds (void) { return FX_Ok; }

const char *MUSIC_ErrorString (int e) { (void)e; return "no music"; }
int MUSIC_Init (int card, const char *params) { (void)card; (void)params; return MUSIC_Error; }
int MUSIC_Shutdown (void) { return MUSIC_Ok; }
const char *MUSIC_GetCurrentDriverName (void) { return "none"; }
void MUSIC_SetVolume (int volume) { (void)volume; }
int MUSIC_SongPlaying (void) { return 0; }
void MUSIC_Continue (void) {}
void MUSIC_Pause (void) {}
int MUSIC_StopSong (void) { return MUSIC_Ok; }
int MUSIC_PlaySong (void *song, unsigned int length, int loopflag) { (void)song; (void)length; (void)loopflag; return MUSIC_Error; }
void MUSIC_RegisterTimbreBank (unsigned char *timbres) { (void)timbres; }

const char *CD_ErrorString (int code) { (void)code; return "no CD"; }
int CD_Init (int card) { (void)card; return CD_Error; }
int CD_GetCurrentDriver (void) { return 0; }
void CD_Shutdown (void) {}
int CD_Play (int track, int loop) { (void)track; (void)loop; return CD_Error; }
void CD_Stop (void) {}
void CD_Pause (int pauseon) { (void)pauseon; }
int CD_IsPlaying (void) { return 0; }
void CD_SetVolume (int volume) { (void)volume; }
