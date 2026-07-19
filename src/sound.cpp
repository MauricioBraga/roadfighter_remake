#include "sdl3_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "sound.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"

#include "filehandling.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

#define MAX_CHANNELS 32

bool sound_enabled=false;

static MIX_Mixer *g_mixer=0;
static MIX_Track *channel_tracks[MAX_CHANNELS]={0};
static int n_channels=-1;

static MIX_Track *music_track=0;
static MIX_Audio *music_audio=0;

// allows to mute all SFX during playing of levelcomplete music.
static bool sfx_muted=false;

void Sound_mute_sfx(bool mute)
{
	sfx_muted=mute;
} /* Sound_mute_sfx */

static int find_free_channel(void)
{
	static int rr=0;
	int i;

	for(i=0;i<n_channels;i++) {
		if (channel_tracks[i]!=0 && !MIX_TrackPlaying(channel_tracks[i])) return i;
	} /* for */

	/* nothing free: steal the next one round-robin, just like Mix_PlayChannel(-1,...)
	   would eventually reuse a channel when they are all busy */
	rr=(rr+1)%(n_channels>0 ? n_channels : 1);
	return rr;
} /* find_free_channel */

static MIX_Track *get_channel_track(int channel)
{
	if (channel<0 || channel>=n_channels) return 0;
	if (channel_tracks[channel]==0) channel_tracks[channel]=MIX_CreateTrack(g_mixer);
	return channel_tracks[channel];
} /* get_channel_track */


bool Sound_initialization(void)
{
	if (-1==Sound_initialization(0,0)) return false;
	return true;
} /* Sound_initialization */


int Sound_initialization(int nc,int nrc)
{
	int i;

	n_channels=8;

	sound_enabled=true;

	if (!MIX_Init()) {
		sound_enabled=false;
#ifdef __DEBUG_MESSAGES
		output_debug_message("Unable to initialize SDL_mixer: %s\n", SDL_GetError());
		output_debug_message("Running the game without audio.\n");
#endif
		return -1;
	} /* if */

	g_mixer=MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
	if (g_mixer==0) {
		sound_enabled=false;
#ifdef __DEBUG_MESSAGES
		output_debug_message("Unable to open audio: %s\n", SDL_GetError());
		output_debug_message("Running the game without audio.\n");
#endif
		return -1;
	} /* if */

	if (nc>0) n_channels=nc;
	if (n_channels>MAX_CHANNELS) n_channels=MAX_CHANNELS;

	for(i=0;i<n_channels;i++) channel_tracks[i]=MIX_CreateTrack(g_mixer);

	(void)nrc; /* reserved channels have no direct equivalent; ignored, as
	              the game never relies on true channel reservation */

	return n_channels;
} /* Sound_init */


void Sound_release(void)
{
	Sound_release_music();
	if (sound_enabled) {
		int i;
		for(i=0;i<n_channels;i++) {
			if (channel_tracks[i]!=0) { MIX_DestroyTrack(channel_tracks[i]); channel_tracks[i]=0; }
		} /* for */
		if (g_mixer!=0) { MIX_DestroyMixer(g_mixer); g_mixer=0; }
		MIX_Quit();
	} /* if */
	sound_enabled=false;
} /* Sound_release */


void Stop_playback(void)
{
	if (sound_enabled) {
		Sound_pause_music();
		Sound_release();
		sound_enabled=false;
	} /* if */
} /* Stop_playback */

void Resume_playback(void)
{
	Resume_playback(0,0);
} /* Resume_playback */


int Resume_playback(int nc,int nrc)
{
	int r=Sound_initialization(nc,nrc);
	Sound_unpause_music();
	return r;
} /* Resume_playback */


/* a check to see if file is readable and greater than zero */
int file_check(const char *fname)
{
	FILE *fp;

	if ((fp=f1open(fname, "r", GAMEDATA))!=NULL) {
		if (fseek(fp,0L, SEEK_END)==0 && ftell(fp)>0) {
			fclose(fp);
			return true;
		} /* if */
		/* either the file could not be read (==-1) or size was zero (==0) */
#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in file_check(): the file %s is corrupted.\n", fname);
#endif
		fclose(fp);
		exit(1);
	} /* if */
	return false;
} /* file_check */



SOUNDT Sound_create_sound(const char *file)
{
	int n_ext=6;
	const char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			if (file_check(name)) return MIX_LoadAudio(g_mixer,name,true);
		} /* for */

#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_sound(): Could not load sound file: %s.(wav|ogg|mp3)\n",file);
#endif
		exit(1);
	} else {
		return 0;
	} /* if */
} /* Sound_create_sound */


void Sound_delete_sound(SOUNDT s)
{
	if (sound_enabled && s!=0) MIX_DestroyAudio(s);
} /* Sound_delete_sound */


int Sound_play(SOUNDT s)
{
	if (sfx_muted) return -1;

	if (sound_enabled && s!=0) {
		int ch=find_free_channel();
		MIX_Track *t=get_channel_track(ch);
		if (t==0) return -1;
		MIX_SetTrackAudio(t,s);
		MIX_SetTrackFrequencyRatio(t,1.0F);
		MIX_SetTrackStereo(t,NULL);
		// restore normal gain (in case it was changed by 
		// the user by pressing + or - to control engine sound..
		MIX_SetTrackGain(t,1.0F);
		MIX_PlayTrack(t,0);
		return ch;
	} /* if */
	return -1;
} /* Sound_play */


int Sound_play(SOUNDT s,int volume)
{
	int ch=Sound_play(s);
	if (ch!=-1) MIX_SetTrackGain(get_channel_track(ch), volume/128.0F);
	return ch;
} /* Sound_play */


int Sound_play_continuous(SOUNDT s)
{
	return Sound_play_loop_ch(s,-1,-1);
} /* Sound_play_continuous */


int Sound_play_continuous(SOUNDT s,int volume)
{
	int ch=Sound_play_continuous(s);
	if (ch!=-1) MIX_SetTrackGain(get_channel_track(ch), volume/128.0F);
	return ch;
} /* Sound_play_continuous */


void Sound_play_ch(SOUNDT s,int ch)
{
	if (sfx_muted) return;
	if (sound_enabled && ch>=0 && ch<n_channels) {
		MIX_Track *t=get_channel_track(ch);
		if (t==0) return;
		MIX_SetTrackAudio(t,s);
		MIX_SetTrackFrequencyRatio(t,1.0F);
		MIX_SetTrackStereo(t,NULL);
		MIX_PlayTrack(t,0);
	} /* if */
} /* Sound_play_ch */


void Sound_play_ch(SOUNDT s,int ch,int volume)
{
	if (sound_enabled && ch>=0 && ch<n_channels) {
		Sound_play_ch(s,ch);
		MIX_SetTrackGain(get_channel_track(ch), volume/128.0F);
	} /* if */
} /* Sound_play_ch */


int Sound_play_loop_ch(SOUNDT s,int channel,int loops)
{
	if (sfx_muted) return -1;
	if (!sound_enabled || s==0) return -1;

	int ch = (channel==-1) ? find_free_channel() : channel;
	MIX_Track *t=get_channel_track(ch);
	if (t==0) return -1;

	MIX_SetTrackAudio(t,s);

	// make sure that every new sound to be played will
	// have the standard volume, not inheriting the volume
	// used by the sound engine.
	MIX_SetTrackFrequencyRatio(t,1.0F);
	MIX_SetTrackStereo(t,NULL);
	MIX_SetTrackGain(t,1.0F);

	/* MIX_SetTrackLoops() only affects a track that is ALREADY playing;
	   calling it before MIX_PlayTrack() has no effect, since MIX_PlayTrack()
	   with options=0 (re)starts the track with the *default* loop count
	   (0 = play once), silently discarding whatever was requested here.
	   The loop count must be passed as a MIX_PlayTrack() property instead. */ 
	SDL_PropertiesID props=SDL_CreateProperties();
	SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,loops);
	MIX_PlayTrack(t,props);
	SDL_DestroyProperties(props);

	// original code:
	// MIX_SetTrackLoops(t,loops);  // <-- track still not being played here
	// MIX_PlayTrack(t,0);  // <-- isso (re)inicia com valores padrão, sobrescrevendo o loop configurado acima


	return ch;
} /* Sound_play_loop_ch */


void Sound_halt_channel(int channel)
{
	if (channel<0 || channel>=n_channels) return;
	if (channel_tracks[channel]!=0) MIX_StopTrack(channel_tracks[channel],0);
} /* Sound_halt_channel */


void Sound_halt_all(void)
{
	int i;
	for(i=0;i<n_channels;i++) Sound_halt_channel(i);
} /* Sound_halt_all */


void Sound_set_channel_frequency(int channel,float ratio)
{
	if (channel<0 || channel>=n_channels) return;
	if (channel_tracks[channel]!=0) MIX_SetTrackFrequencyRatio(channel_tracks[channel],ratio);
} /* Sound_set_channel_frequency */


void Sound_set_channel_pan(int channel,float left,float right)
{
	MIX_StereoGains gains;

	if (channel<0 || channel>=n_channels) return;
	if (channel_tracks[channel]==0) return;

	gains.left=left;
	gains.right=right;
	MIX_SetTrackStereo(channel_tracks[channel],&gains);
} /* Sound_set_channel_pan */

void Sound_set_channel_gain(int channel,float gain)
{

	if (channel<0 || channel>=n_channels) return;
	if (channel_tracks[channel]!=0) MIX_SetTrackGain(channel_tracks[channel],gain);
} /* Sound_set_channel_gain */


bool Sound_channel_playing(int channel)
{
	if (channel<0 || channel>=n_channels || channel_tracks[channel]==0) return false;
	return MIX_TrackPlaying(channel_tracks[channel]);
} /* Sound_channel_playing */


bool Sound_any_playing(void)
{
	int i;

	if (!sound_enabled) return false;
	for(i=0;i<n_channels;i++) {
		if (channel_tracks[i]!=0 && MIX_TrackPlaying(channel_tracks[i])) return true;
	} /* for */
	return false;
} /* Sound_any_playing */


MIX_Audio *Sound_create_stream(const char *file)
{
	int n_ext=6;
	const char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			if (file_check(name)) return MIX_LoadAudio(g_mixer,name,false);
		} /* for */

#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_stream(): Could not load sound file: %s.(wav|ogg|mp3)\n", file);
#endif
		exit(1);
	} else {
		return 0;
	} /* if */
} /* Sound_create_stream */


void Sound_create_music(const char *f1,int loops)
{
	if (sound_enabled) {
		Sound_release_music();
		if (f1!=0) {
			music_audio=Sound_create_stream(f1);
			music_track=MIX_CreateTrack(g_mixer);
			if (music_track!=0 && music_audio!=0) {
				SDL_PropertiesID props;

				MIX_SetTrackAudio(music_track,music_audio);

				/* See Sound_play_loop_ch() for why the loop count has to be
				   passed as a MIX_PlayTrack() property instead of via
				   MIX_SetTrackLoops() beforehand. */ 
				props=SDL_CreateProperties();
				SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,loops);
				MIX_PlayTrack(music_track,props);
				SDL_DestroyProperties(props);
			} /* if */
		} else {
			music_audio=0;
			music_track=0;
		} /* if */
	} /* if */
} /* Sound_create_music */


bool Sound_file_test(const char *f1)
{
	int n_ext=6;
	const char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,f1);
			strcat(name,ext[i]);
			if (file_check(name)) return true;
		} /* for */

		return false;
	} else {
		return false;
	} /* if */
} /* Sound_file_test */


void Sound_release_music(void)
{
	if (sound_enabled) {
		if (music_track!=0) { MIX_StopTrack(music_track,0); MIX_DestroyTrack(music_track); music_track=0; }
		if (music_audio!=0) { MIX_DestroyAudio(music_audio); music_audio=0; }
	} /* if */
} /* Sound_release_music */



void Sound_pause_music(void)
{
	if (music_track!=0) MIX_PauseTrack(music_track);
} /* Sound_pause_music */


void Sound_unpause_music(void)
{
	if (music_track!=0) MIX_ResumeTrack(music_track);
} /* Sound_unpause_music */


void Sound_music_volume(int volume)
{
	if (volume<0) volume=0;
	if (volume>127) volume=127;
	if (music_track!=0) MIX_SetTrackGain(music_track, volume/127.0F);
} /* Sound_music_volume */
