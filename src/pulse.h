//
// Created by cmh on 3/2/26.
//

#ifndef PNMIXER_PULSE_H
#define PNMIXER_PULSE_H

struct pa_mainloop_api;
struct pa_glib_mainloop;
struct pa_context;

typedef enum result {
    SUCCESS,
    FAILURE,
} EResult;

typedef struct pulseaudioinfo {
    gchar *default_sink_name;
    gchar *server_name;
    gchar *default_source_name;
    gchar *user_name;
} SPulseAudioInfo;

typedef struct pulseaudiostate {
    pa_mainloop_api* mainloop_api;
    pa_glib_mainloop* mainloop;
    pa_context* context;

    SPulseAudioInfo* info;
    gboolean has_sink_name;

    // test loop state
    gboolean ready;
    gboolean failed;

    GMainLoop* loop;
} SPulseAudioState;

SPulseAudioState* try_init_pulseaudio();
void try_context_connect(SPulseAudioState* pulse_audio_state);
void on_context_state_changed(pa_context *context, void *user_data);
void on_context_ready(SPulseAudioState* pulse_audio_state);
void free_pulseaudio(SPulseAudioState* state);

#endif //PNMIXER_PULSE_H