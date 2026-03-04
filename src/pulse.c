//
// Created by Djlanav on 3/2/26.
//

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <syslog.h>
#include <glib.h>
#include "pulse.h"

#define DO_LOOP_TEST 1

/* Private functions */

static gboolean
main_loop_timeout(void* user_data)
{
    SPulseAudioState* pa_state = user_data;
    syslog(LOG_USER | LOG_ERR, "PulseAudio timeout");
    pa_state->failed = true;
    if (pa_state->loop) g_main_loop_quit(pa_state->loop);
    return G_SOURCE_REMOVE;
}

static void
on_context_get_sink_info(pa_context *context, const pa_sink_info *info, gint32 eol, void *user_data)
{
    syslog(LOG_USER | LOG_DEBUG, "PulseAudio get sink info");

    if (eol <= 0) {
        pa_cvolume vol = info->volume;
        guint32 avg_vol = pa_cvolume_avg(&vol) * 100ULL / PA_VOLUME_NORM;
        syslog(LOG_USER | LOG_INFO, "Average volume: %d", avg_vol);
    }
}

static void
on_context_get_server_info(pa_context *context, const pa_server_info *info, void *user_data)
{
    SPulseAudioState *pulse_audio_state = user_data;
    SPulseAudioInfo *pulse_audio_info;

    if (pulse_audio_state->info != NULL) {
        pulse_audio_info = pulse_audio_state->info;
    } else {
        pulse_audio_info = g_new(SPulseAudioInfo, 1);
    }

    pulse_audio_info->default_sink_name = g_strdup(info->default_sink_name);
    pulse_audio_info->server_name = g_strdup(info->server_name);
    pulse_audio_state->info = pulse_audio_info;

    syslog(LOG_USER | LOG_INFO, "Server name: %s", pulse_audio_state->info->server_name);
    syslog(LOG_USER | LOG_INFO, "Default sink name: %s", pulse_audio_state->info->default_sink_name);
    pa_context_get_sink_info_by_name(pulse_audio_state->context, pulse_audio_state->info->default_sink_name,
        on_context_get_sink_info, NULL);
}

/* Public functions */

SPulseAudioState*
try_init_pulseaudio()
{
    SPulseAudioState *state = g_new(SPulseAudioState, 1);
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
    pa_context_set_state_callback(context, on_context_state_changed, state);

    state->mainloop = mainloop;
    state->mainloop_api = mainloop_api;
    state->context = context;

    syslog(LOG_USER | LOG_INFO, "PulseAudio initialized");
    return state;
}

void
try_context_connect(SPulseAudioState *pulse_audio_state)
{
    const int connect_status = pa_context_connect(pulse_audio_state->context,
        NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
    if (connect_status < 0) {
        const char *error_status = pa_strerror (connect_status);
        syslog (LOG_USER | LOG_ERR, "Failed to connect to pulseaudio server: %s", error_status);
        return;
    }

    // loop
#if DO_LOOP_TEST
    pulse_audio_state->loop = g_main_loop_new(NULL, false);
    g_timeout_add(500, main_loop_timeout, pulse_audio_state);
    g_main_loop_run(pulse_audio_state->loop);
    g_main_loop_unref(pulse_audio_state->loop);
    pulse_audio_state->loop = NULL;
#endif // DO_LOOP_TEST
}

void
on_context_get_sink_info_list(pa_context *context, const pa_sink_info *info, void *user_data) {
    // TODO
}

void
pulse_audio_get_server_info(SPulseAudioState *pulse_audio_state)
{
    pa_context_get_server_info(pulse_audio_state->context, on_context_get_server_info, pulse_audio_state);
}

void
on_context_state_changed(pa_context *context, void *user_data)
{
    SPulseAudioState* pa_state = user_data;
    const pa_context_state_t state = pa_context_get_state(pa_state->context);
    switch (state) {
        case PA_CONTEXT_CONNECTING:
            syslog(LOG_USER | LOG_INFO, "PulseAudio connecting");
            break;
        case PA_CONTEXT_READY:
            syslog(LOG_USER | LOG_INFO, "PulseAudio ready");
            on_context_ready(pa_state);
            break;
        case PA_CONTEXT_FAILED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio failed to connect");
            pa_state->failed = true;
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
on_context_ready(SPulseAudioState *pulse_audio_state)
{
    pulse_audio_state->ready = true;
    pulse_audio_get_server_info(pulse_audio_state);
}

void
free_pulseaudio(SPulseAudioState *state)
{
    pa_context_disconnect(state->context);
    syslog(LOG_USER | LOG_INFO, "Exiting PulseAudio");

    pa_glib_mainloop_free(state->mainloop);
    closelog ();
    g_free((SPulseAudioInfo*)state->info);
    g_free((SPulseAudioState*)state);
}