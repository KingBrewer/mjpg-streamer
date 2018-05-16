#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <syslog.h>
#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include "utils.h"
#include "mjpgstreamer.h"

int main(int argc, char *argv[])
{
    (void) argc;
    (void*) argv;

    char *input_plugin = "input_uvc.so";
    char *input_params  = "--resolution 640x480 --fps 5 --device /dev/video1";

    char *output_plugin = "output_http.so";
    char *output_params = "--port 8080";

    run_streamer(input_plugin, input_params, output_plugin, output_params);


    /* wait for signals */
    pause();

    return 0;
}
