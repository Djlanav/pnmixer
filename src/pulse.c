//
// Created by Djlanav on 3/2/26.
//

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <syslog.h>
#include <glib.h>
#include "audio.h"
#include "pulse.h"

/* Private functions */

static void
on_context_get_sink_info(G_GNUC_UNUSED pa_context *context, const pa_sink_info *info, gint32 eol, void *user_data)
{
    Audio *audio = user_data;
    SPulseAudioState *pa_state = audio->pa_state;
    pa_state->sink_info = g_new(pa_sink_info, 1);

    if (eol <= 0) {
        memcpy(pa_state->sink_info, info, sizeof(pa_sink_info));
        // guint32 avg_vol = pa_cvolume_avg(&vol) * 100 / PA_VOLUME_NORM;
        // syslog(LOG_USER | LOG_INFO, "Average volume: %d", avg_vol);
    }

    if (pa_state->operation == PA_OPERATION_RELOAD || pa_state->operation == PA_GET_SINK_INFO) {
        pa_state->phase = OPERATION_COMPLETE;
    }
}

static void
on_context_get_server_info(G_GNUC_UNUSED pa_context *context, const pa_server_info *info, void *user_data)
{
    Audio *audio = user_data;
    SPulseAudioState *pa_state = audio->pa_state;

    pa_state->server_info = g_new(pa_server_info, 1);
    memcpy(pa_state->server_info, info, sizeof(pa_server_info));

    syslog(LOG_USER | LOG_INFO, "Server name: %s", pa_state->server_info->server_name);
    syslog(LOG_USER | LOG_INFO, "Default sink name: %s", pa_state->server_info->default_sink_name);

    pa_state->phase = SERVER_INFO_READY;

    if (pa_state->operation == PA_OPERATION_RELOAD || pa_state->operation == PA_GET_SINK_INFO) {
        pa_state->phase = GETTING_SINK_INFO;
        pulseaudio_get_sink_info_by_name(audio, pa_state->server_info->default_sink_name);
    }
}

static void
pulse_audio_state_destructor(SPulseAudioState *pulse_audio_state)
{
    g_free(pulse_audio_state->info);
    g_free(pulse_audio_state->sink_info);
    g_free(pulse_audio_state->server_info);
    g_free(pulse_audio_state);
}

static void
on_context_success(G_GNUC_UNUSED pa_context *context, G_GNUC_UNUSED gint32 success, void *user_data)
{
    AudioUserData *data = user_data;
    Audio *audio = data->audio;
    SPulseAudioState *pa_state = audio->pa_state;
    syslog(LOG_USER | LOG_INFO, "PulseAudio set volume");

    if (pa_state->operation == PA_SET_VOLUME && pa_state->phase == SETTING_SINK_VOLUME) {
        invoke_handlers(audio, data->signal, data->user);
    }

    if (pa_state->phase == OPERATION_COMPLETE) {
        g_slist_free(data->extra);
        g_free(data);
    }
}

/* Public functions */

void
pulseaudio_get_sink_info_by_name(Audio *audio, const gchar *sink_name)
{
    SPulseAudioState *pa_state = audio->pa_state;
    pa_context_get_sink_info_by_name(pa_state->context, sink_name, on_context_get_sink_info, audio);
}

void
pulseaudio_get_sink_info_by_index(Audio *audio, const guint32 index)
{
    SPulseAudioState *pa_state = audio->pa_state;
    pa_context_get_sink_info_by_index(pa_state->context, index, on_context_get_sink_info, audio);
}

void
pulseaudio_set_sink_volume_by_name(Audio *audio, AudioUserData *user_data, const gchar *sink_name)
{
    SPulseAudioState *pa_state = audio->pa_state;
    GSList* extra = user_data->extra;
    pa_cvolume* volume = g_slist_nth_data(extra, 0);

    pa_state->phase = SETTING_SINK_VOLUME;
    pa_context_set_sink_volume_by_name(pa_state->context, sink_name, volume, on_context_success, user_data);
}

SPulseAudioState*
init_pulseaudio(Audio *audio)
{
    SPulseAudioState *state = g_new(SPulseAudioState, 1);
    state->is_context_connected = false;
    state->destructor = pulse_audio_state_destructor;

    openlog("pnmixer-pulse", LOG_PID | LOG_CONS, LOG_USER);

    pa_glib_mainloop* mainloop = pa_glib_mainloop_new(NULL);
    if (!mainloop) {
        syslog(LOG_USER | LOG_ERR, "Failed to create PulseAudio mainloop");
        return NULL;
    }

    pa_mainloop_api *mainloop_api = pa_glib_mainloop_get_api(mainloop);
    if (!mainloop_api) {
        syslog(LOG_USER | LOG_ERR, "Failed to create PulseAudio mainloop_api");
        return NULL;
    }

    pa_context* context = pa_context_new(mainloop_api, "pnmixer-pulse");
    if (!context) {
        syslog(LOG_USER | LOG_ERR, "Failed to create PulseAudio context");
        return NULL;
    }
    pa_context_set_state_callback(context, on_context_state_changed, audio);

    state->mainloop = mainloop;
    state->mainloop_api = mainloop_api;
    state->context = context;

    syslog(LOG_USER | LOG_INFO, "PulseAudio initialized");
    return state;
}

void
try_context_connect(Audio *audio)
{
    SPulseAudioState *pulse_audio_state = audio->pa_state;
    pulse_audio_state->phase = CONNECTING_SERVER;

    const int connect_status = pa_context_connect(pulse_audio_state->context,
        NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
    if (connect_status < 0) {
        const char *error_status = pa_strerror (connect_status);
        syslog (LOG_USER | LOG_ERR, "Failed to connect to PulseAudio server: %s", error_status);
    } else {
        syslog(LOG_USER | LOG_INFO, "Connected to PulseAudio server");
        pulse_audio_state->phase = SERVER_CONNECTED;
        pulse_audio_state->is_context_connected = true;
    }
}

void
on_context_get_sink_info_list(pa_context *context, const pa_sink_info *info, void *user_data) {
    // TODO
}

void
pulseaudio_get_server_info(Audio *audio)
{
    SPulseAudioState *pulse_audio_state = audio->pa_state;
    pulse_audio_state->phase = GETTING_SERVER_INFO;
    pa_context_get_server_info(pulse_audio_state->context, on_context_get_server_info, audio);
}

void
on_context_state_changed(pa_context *context, void *user_data)
{
    Audio *audio = user_data;
    SPulseAudioState *pa_state = audio->pa_state;

    const pa_context_state_t state = pa_context_get_state(pa_state->context);
    switch (state) {
        case PA_CONTEXT_CONNECTING:
            syslog(LOG_USER | LOG_INFO, "PulseAudio connecting");
            break;
        case PA_CONTEXT_READY:
            syslog(LOG_USER | LOG_INFO, "PulseAudio ready");
            on_context_ready(audio);
            break;
        case PA_CONTEXT_FAILED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio failed to connect");
            if (pa_state->loop) g_main_loop_quit(pa_state->loop);
            break;
        case PA_CONTEXT_TERMINATED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio terminated");
            if (pa_state->loop) g_main_loop_quit(pa_state->loop);
            break;
        case PA_CONTEXT_AUTHORIZING:
            syslog(LOG_USER | LOG_INFO, "PulseAudio authorizing");
            break;
        case PA_CONTEXT_SETTING_NAME:
            syslog(LOG_USER | LOG_INFO, "PulseAudio setting name");
            break;
        case PA_CONTEXT_UNCONNECTED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio unconnected");
            break;
    }
}

void
on_context_ready(Audio *audio)
{
    pulseaudio_get_server_info(audio);
}

void
free_pulseaudio(Audio *audio)
{
    SPulseAudioState *state = audio->pa_state;
    pa_context_disconnect(state->context);
    syslog(LOG_USER | LOG_INFO, "Exiting PulseAudio");

    pa_glib_mainloop_free(state->mainloop);
    closelog ();

    state->destructor(state);
}