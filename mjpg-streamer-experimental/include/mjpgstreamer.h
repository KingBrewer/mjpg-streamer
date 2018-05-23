#ifndef MJPGSTREAMER_H
#define MJPGSTREAMER_H

int run_streamer(const char* in_plugin, const char* in_plugin_params, const char* out_plugin, const char* out_plugin_params);
int stop_streamer();

#endif
