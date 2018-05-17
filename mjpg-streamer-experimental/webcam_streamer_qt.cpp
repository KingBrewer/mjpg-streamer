#include <QCoreApplication>
#include <QDebug>
#include <QNetworkInterface>


extern "C" {
#include  "mjpgstreamer.h"
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    char *input_plugin = "input_raspicam.so";
    char *input_params  = "-x 800 -y 600 -fps 10 -ex nightpreview";

    char *output_plugin = "output_http.so";
    char *output_params = "--port 8080";

    run_streamer(input_plugin, input_params, output_plugin, output_params);

    return a.exec();
}
