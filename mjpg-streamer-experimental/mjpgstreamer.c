/*******************************************************************************
#                                                                              #
#      MJPG-streamer allows to stream JPG frames from an input-plugin          #
#      to several output plugins                                               #
#                                                                              #
#      Copyright (C) 2007 Tom Stöveken                                         #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/

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
#include "mjpg_streamer.h"

/* globals */
static globals global;

int run_streamer(const char* in_plugin, const char* in_plugin_params, const char* out_plugin, const char* out_plugin_params)
{
    unsigned int i = 0;
    unsigned int j = 0;
    global.incnt = 1;
    global.outcnt = 1;

    /* this mutex and the conditional variable are used to synchronize access to the global picture buffer */
    if(pthread_mutex_init(&global.in[i].db, NULL) != 0) {
        LOG("could not initialize mutex variable\n");
        closelog();
        return EXIT_FAILURE;
    }
    if(pthread_cond_init(&global.in[i].db_update, NULL) != 0) {
        LOG("could not initialize condition variable\n");
        closelog();
        return EXIT_FAILURE;
    }

    global.in[i].stop      = 0;
    global.in[i].context   = NULL;
    global.in[i].buf       = NULL;
    global.in[i].size      = 0;
    global.in[i].plugin = strdup(in_plugin);
    global.in[i].handle = dlopen(global.in[i].plugin, RTLD_LAZY);
    if(!global.in[i].handle) {
        LOG("ERROR: could not find input plugin\n");
        LOG("       Perhaps you want to adjust the search path with:\n");
        LOG("       # export LD_LIBRARY_PATH=/path/to/plugin/folder\n");
        LOG("       dlopen: %s\n", dlerror());
        closelog();
        return EXIT_FAILURE;
    }
    global.in[i].init = dlsym(global.in[i].handle, "input_init");
    if(global.in[i].init == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }
    global.in[i].stop = dlsym(global.in[i].handle, "input_stop");
    if(global.in[i].stop == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }
    global.in[i].run = dlsym(global.in[i].handle, "input_run");
    if(global.in[i].run == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }
    /* try to find optional command */
    global.in[i].cmd = dlsym(global.in[i].handle, "input_cmd");

    global.in[i].param.parameters = (char*)in_plugin_params;

    for (j = 0; j<MAX_PLUGIN_ARGUMENTS; j++) {
        global.in[i].param.argv[j] = NULL;
    }

    split_parameters(global.in[i].param.parameters, &global.in[i].param.argc, global.in[i].param.argv);
    global.in[i].param.global = &global;
    global.in[i].param.id = i;

    if(global.in[i].init(&global.in[i].param, i)) {
        LOG("input_init() return value signals to exit\n");
        closelog();
        return EXIT_FAILURE;
    }

    // --------------------------------------------------
    //* open output plugin */
    // --------------------------------------------------
    global.out[i].plugin = strdup(out_plugin);
    global.out[i].handle = dlopen(global.out[i].plugin, RTLD_LAZY);
    if(!global.out[i].handle) {
        LOG("ERROR: could not find output plugin %s\n", global.out[i].plugin);
        LOG("       Perhaps you want to adjust the search path with:\n");
        LOG("       # export LD_LIBRARY_PATH=/path/to/plugin/folder\n");
        LOG("       dlopen: %s\n", dlerror());
        closelog();
        return EXIT_FAILURE;
    }
    global.out[i].init = dlsym(global.out[i].handle, "output_init");
    if(global.out[i].init == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }
    global.out[i].stop = dlsym(global.out[i].handle, "output_stop");
    if(global.out[i].stop == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }
    global.out[i].run = dlsym(global.out[i].handle, "output_run");
    if(global.out[i].run == NULL) {
        LOG("%s\n", dlerror());
        return EXIT_FAILURE;
    }

    /* try to find optional command */
    global.out[i].cmd = dlsym(global.out[i].handle, "output_cmd");

    global.out[i].param.parameters = (char*)out_plugin_params;

    for (j = 0; j<MAX_PLUGIN_ARGUMENTS; j++) {
        global.out[i].param.argv[j] = NULL;
    }
    split_parameters(global.out[i].param.parameters, &global.out[i].param.argc, global.out[i].param.argv);

    global.out[i].param.global = &global;
    global.out[i].param.id = i;
    if(global.out[i].init(&global.out[i].param, i)) {
        LOG("output_init() return value signals to exit\n");
        closelog();
        return EXIT_FAILURE;
    }

    // --------------------------------------------------
    // starting plugins
    // --------------------------------------------------

    /* start to read the input, push pictures into global buffer */
    DBG("starting %d input plugin\n", global.incnt);

    syslog(LOG_INFO, "starting input plugin %s", global.in[i].plugin);
    if(global.in[i].run(i)) {
        LOG("can not run input plugin %d: %s\n", i, global.in[i].plugin);
        closelog();
        return EXIT_FAILURE;
    }

    DBG("starting %d output plugin(s)\n", global.outcnt);
    for(i = 0; i < global.outcnt; i++) {
        syslog(LOG_INFO, "starting output plugin: %s (ID: %02d)", global.out[i].plugin, global.out[i].param.id);
        global.out[i].run(global.out[i].param.id);
    }

    return 0;
}
