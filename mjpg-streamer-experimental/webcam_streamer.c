#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "utils.h"
#include "mjpgstreamer.h"

static void signal_handler(int sig)
{
    (void) sig;
    stop_streamer();
    return;
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void*) argv;

    char *input_plugin = "input_uvc.so";
    char *input_params  = "--resolution 640x480 --fps 5 --device /dev/video1";

    char *output_plugin = "output_http.so";
    char *output_params = "--port 8080";

    /* ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    /* register signal handler for <CTRL>+C in order to clean up */
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }

    run_streamer(input_plugin, input_params, output_plugin, output_params);

    /* wait for signals */
    pause();

    return 0;
}
