#include <utils/network.h>

#ifndef utils_h
#define utils_h

String getSysMsg() {
    String msg;
    if (APisFallback) msg = "<span style='color: red'>Attempting to connect to WiFi network '" + parseNetConfig(4) + "' has failed.</span>";

    return msg;
}

#endif utils_h
