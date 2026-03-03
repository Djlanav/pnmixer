//
// Created by cmh on 3/2/26.
//

#ifndef PNMIXER_PULSE_H
#define PNMIXER_PULSE_H

struct pa_mainloop_api;
struct pa_glib_mainloop;
struct pa_context;

typedef enum pulseaudioinitstatus {
    PULSEAUDIO_INIT_STATUS_INITIALIZED,
    MAINLOOP_INIT_FAILED,
    MAINLOOP_API_INIT_FAILED,
    CONTEXT_INIT_FAILED,
} EPulseAudioInitStatus;

typedef enum result {
    SUCCESS,
    FAILURE,
} EResult;

typedef struct pulseaudiostate {
    pa_mainloop_api* mainloop_api;
    pa_glib_mainloop* mainloop;
    pa_context* context;
} SPulseAudioState;

typedef struct pulseaudioinfo {
    gchar *default_sink_name;
    gchar *server_name;
    gchar *default_source_name;
    gchar *user_name;
} SPulseAudioInfo;

void on_context_state_changed(pa_context *context, void *user_data);

#endif //PNMIXER_PULSE_H