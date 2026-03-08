//
// Created by cmh on 3/2/26.
//

#ifndef PNMIXER_PULSE_H
#define PNMIXER_PULSE_H

struct pa_mainloop_api;
struct pa_glib_mainloop;
struct pa_context;

typedef struct audio Audio;

typedef enum result {
    SUCCESS,
    FAILURE,
} EResult;

typedef enum e_pulseaudio_operation {
    PA_OPERATION_RELOAD,
    PA_GET_SINK_INFO,
    PA_SET_VOLUME,
    PA_CHECK_FOR_MUTED,
    PA_GET_VOLUME,
} PulseAudioOperation;

typedef enum e_pulseaudio_phase {
    CONNECTING_SERVER,
    SERVER_CONNECTED,
    GETTING_SERVER_INFO,
    SERVER_INFO_READY,
    GETTING_SINK_INFO,
    SINK_INFO_READY,
    SETTING_LOCAL_VOLUME,
    SETTING_SINK_VOLUME,
    GETTING_SINK_VOLUME,
    CHECKING_MUTED,
    OPERATION_COMPLETE,
} PulseAudioPhase;

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
    pa_server_info* server_info;
    pa_sink_info *sink_info;

    PulseAudioOperation operation;
    PulseAudioPhase phase;

    gboolean is_context_connected;

    SPulseAudioInfo* info;

    GMainLoop* loop;

    void(*destructor)(struct pulseaudiostate *this);
} SPulseAudioState;

SPulseAudioState* init_pulseaudio(Audio *audio);
void try_context_connect(Audio *audio);
void on_context_state_changed(pa_context *context, void *user_data);
void on_context_ready(Audio *audio);
void free_pulseaudio(Audio *state);

void pulseaudio_get_server_info(Audio *audio);
void pulseaudio_get_sink_info_by_name(Audio *audio, const gchar *sink_name);
void pulseaudio_get_sink_info_by_index(Audio *audio, guint32 index);
void pulseaudio_set_sink_volume_by_name(Audio *audio, AudioUserData *user_data, const gchar *sink_name);

#endif //PNMIXER_PULSE_H