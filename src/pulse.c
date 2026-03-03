//
// Created by cmh on 3/2/26.
//

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <syslog.h>
#include <glib.h>
#include "pulse.h"

SPulseAudioState*
try_init_pulseaudio()
{
    SPulseAudioState* state = g_new(SPulseAudioState, 1);
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
    pa_context_set_state_callback(context, on_context_state_changed, NULL);

    state->mainloop = mainloop;
    state->mainloop_api = mainloop_api;
    state->context = context;

    syslog(LOG_USER | LOG_INFO, "PulseAudio initialized");
    return state;
}

EResult
try_context_connect (pa_context *context)
{
    const int connect_status = pa_context_connect(context, nullptr, PA_CONTEXT_NOAUTOSPAWN, NULL);
    if (!connect_status) {
        const char *error_status = pa_strerror (connect_status);
        syslog (LOG_USER | LOG_ERR, "Failed to connect to pulseaudio server: %s", error_status);
        return FAILURE;
    }

    return SUCCESS;
}

void
on_context_get_sink_info_list(pa_context *context, const pa_sink_info *info, void *user_data) {
    // TODO
}

void
on_context_get_server_info(pa_context *context, const pa_server_info *info, void *user_data)
{
    SPulseAudioInfo *user_info = user_data;
    const gchar *ctx_server_name = pa_context_get_server(context);
    if (strcmp(ctx_server_name, info->server_name) != 0) {
        syslog(LOG_USER | LOG_WARNING, "PulseAudio server name mismatch");
    }

    strcpy(user_info->server_name, info->server_name);
    strcpy(user_info->default_sink_name, info->default_sink_name);
    strcpy(user_info->default_source_name, info->default_source_name);
    strcpy(user_info->user_name, info->user_name);
}

void
on_context_state_changed(pa_context *context, void *user_data)
{
    const pa_context_state_t state = pa_context_get_state(context);
    switch (state) {
        case PA_CONTEXT_CONNECTING:
            syslog(LOG_USER | LOG_INFO, "PulseAudio connecting");
            break;
        case PA_CONTEXT_READY:
            syslog(LOG_USER | LOG_INFO, "PulseAudio ready");
            break;
        case PA_CONTEXT_FAILED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio failed to connect");
            break;
        case PA_CONTEXT_TERMINATED:
            syslog(LOG_USER | LOG_INFO, "PulseAudio terminated");
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
free_pulseaudio(SPulseAudioState *state)
{
    pa_context_disconnect (state->context);
    syslog(LOG_USER | LOG_INFO, "Exiting PulseAudio");

    pa_glib_mainloop_free(state->mainloop);
    closelog ();
    g_free((SPulseAudioState*)state);
}