/* audio.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file audio.h
 * Header for audio.c.
 * @brief Header for audio.c.
 */

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <glib.h>

/* High-level audio functions, no need to have a soundcard ready for that */

GSList *audio_get_card_list(void);
GSList *audio_get_channel_list(const char *card);

/* Audio status: card & channel name, mute & volume handling.
 * Everyone who changes the volume must say who he is.
 */

typedef enum audio_user {
	AUDIO_USER_UNKNOWN,
	AUDIO_USER_POPUP,
	AUDIO_USER_TRAY_ICON,
	AUDIO_USER_HOTKEYS,
} AudioUser;

/* Soundcard management */

typedef struct alsa_card AlsaCard;
typedef struct pulseaudiostate SPulseAudioState;
typedef struct audio Audio;

typedef enum audio_signal AudioSignal;
enum audio_signal {
	AUDIO_NO_CARD,
	AUDIO_CARD_INITIALIZED,
	AUDIO_CARD_CLEANED_UP,
	AUDIO_CARD_DISCONNECTED,
	AUDIO_CARD_ERROR,
	AUDIO_VALUES_CHANGED,
};

typedef struct audio_vtable {
	void (*free)(Audio *audio);
	void (*reload)(Audio *audio);
	const gchar *(*get_card)(Audio *audio);
	const gchar *(*get_channel)(Audio *audio);
	gboolean (*has_mute)(Audio *audio);
	gboolean (*is_muted)(Audio *audio);
	void (*toggle_mute)(Audio *audio, AudioUser user);
	gdouble (*get_volume)(Audio *audio);
	void (*set_volume)(Audio *audio, AudioUser user, gdouble volume, gint direction);
	void (*lower_volume)(Audio *audio, AudioUser user);
	void (*raise_volume)(Audio *audio, AudioUser user);
} AudioVTable;

/* VTable for ALSA functions */
extern const AudioVTable alsa_vtable;
/* VTable for PulseAudio functions */
extern const AudioVTable pa_vtable;

struct audio {
	/* Backend */
	const AudioVTable *vtable;
	/* Preferences */
	gdouble scroll_step;
	gboolean normalize;
	/* Underlying sound card */
	// as per the comment, this just holds essentially a reference to the current soundcard
	// This struct is found in alsa.c/alsa.h
	AlsaCard *soundcard;
	/* Cached value (to avoid querying the underlying
	 * sound card each time we need the info).
	 */
	gchar *card;
	gchar *channel;
	/* Pulse Audio State (if PulseAudio is available on the system */
	SPulseAudioState *pa_state;
	/* Last action performed (volume/mute change) */
	gint64 last_action_timestamp;
	/* True if we're not working with the preferred card */
	gboolean fallback;
	gint64 fallback_last_check;
	/* User signal handlers.
	 * To be invoked when the audio status changes.
	 */
	GSList *handlers;
};

Audio *audio_new(void);
void select_backend(Audio *audio);
void invoke_handlers(Audio *audio, AudioSignal signal, AudioUser user);

/*
 * Legacy ALSA-based audio functions
 */
void audio_free(Audio *audio);
void audio_reload(Audio *audio);

const char *audio_get_card(Audio *audio);
const char *audio_get_channel(Audio *audio);
gboolean audio_has_mute(Audio *audio);
gboolean audio_is_muted(Audio *audio);
void audio_toggle_mute(Audio *audio, AudioUser user);
gdouble audio_get_volume(Audio *audio);
void audio_set_volume(Audio *audio, AudioUser user, gdouble volume, gint direction);
void audio_lower_volume(Audio *audio, AudioUser user);
void audio_raise_volume(Audio *audio, AudioUser user);

/*
 * PulseAudio functions
 */

typedef struct audio_user_data {
	Audio *audio;
	AudioSignal signal;
	AudioUser user;
	GSList *extra;
} AudioUserData;

void pulseaudio_free(Audio *audio);
void pulseaudio_reload(Audio *audio);
void pulseaudio_set_volume(Audio *audio, AudioUser user, gdouble volume, gint direction);
gdouble pulseaudio_get_volume(Audio *audio);
gboolean pulseaudio_is_muted(Audio *audio);

/* Signal handling.
 * The audio system sends signals out there when something happens.
 */

struct audio_event {
	AudioSignal signal;
	AudioUser user;
	const gchar *card;
	const gchar *channel;
	gboolean has_mute;
	gboolean muted;
	gdouble volume;
};

typedef struct audio_event AudioEvent;

typedef void (*AudioCallback) (Audio *audio, AudioEvent *event, gpointer data);

void audio_signals_connect(Audio *audio, AudioCallback callback, gpointer data);
void audio_signals_disconnect(Audio *audio, AudioCallback callback, gpointer data);

#endif				// _AUDIO_H
