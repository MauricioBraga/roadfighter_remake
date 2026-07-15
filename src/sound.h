#ifndef __BRAIN_SDL_SOUND
#define __BRAIN_SDL_SOUND

/*
 * RoadFighter originally used the classic SDL_mixer 1.2 "Mix_*" API
 * (Mix_OpenAudio/Mix_PlayChannel/Mix_Chunk/...). SDL3_mixer replaced that
 * whole API with a new track-based model (MIX_Mixer / MIX_Audio /
 * MIX_Track). This header keeps the exact same Sound_* interface the rest
 * of the game already calls, and a few extra small helpers
 * (Sound_play_loop_ch/Sound_set_channel_frequency/Sound_set_channel_pan/
 * Sound_any_playing) that replace direct Mix_* calls that a couple of
 * files used to make for the engine/skid pitch-bend effect.
 */

#ifndef MIX_MAX_VOLUME
#define MIX_MAX_VOLUME 128 /* matches classic SDL_mixer's scale, used by Sound_play(s,volume) */
#endif

typedef struct MIX_Audio *SOUNDT;

bool Sound_initialization(void);
int Sound_initialization(int nc,int nrc);
void Sound_release(void);

bool Sound_file_test(const char *f1);

SOUNDT Sound_create_sound(const char *file);
void Sound_delete_sound(SOUNDT s);
int Sound_play(SOUNDT s);
int Sound_play(SOUNDT s,int volume);
int Sound_play_continuous(SOUNDT s);
int Sound_play_continuous(SOUNDT s,int volume);
void Sound_play_ch(SOUNDT s,int channel);
void Sound_play_ch(SOUNDT s,int channel,int volume);

/* Extra helpers used for the procedurally pitch-shifted engine/skid sounds
 * (CPlayerCarObject) that in the original code manually resampled a raw
 * Mix_Chunk buffer every frame. MIX_SetTrackFrequencyRatio() makes that
 * unnecessary: play the sound looped once, then just adjust its playback
 * rate/pan every frame. */
int Sound_play_loop_ch(SOUNDT s,int channel,int loops);
void Sound_halt_channel(int channel);
void Sound_set_channel_frequency(int channel,float ratio);
void Sound_set_channel_pan(int channel,float left,float right);
bool Sound_channel_playing(int channel);
bool Sound_any_playing(void);

void Sound_create_music(const char *f1,int times);
void Sound_release_music(void);
void Sound_pause_music(void);
void Sound_unpause_music(void);

void Sound_music_volume(int volume);

/* These functions are AGRESIVE! (i.e. they actually STOP the mixer and restart it) */
void Stop_playback(void);
void Resume_playback(void);
int Resume_playback(int nc,int nr);

#endif
