#ifndef utils_h
#define utils_h

#include <utils/network.h>
#include "rom/rtc.h"

bool globalErrorOverride = false;
bool bigError = false;

String getSysMsg() {
    String msg = "";
    bool isError = false;

    if (APisFallback && !bigError) {
        msg = "Could not connect to WiFi network '" + parseNetConfig(4) + "'."; isError = true;
    } else if (!NTPisValid && !bigError) {
        msg = "NTP server is unresponsive."; isError = true;
    } else {
        if (!globalErrorOverride) {
            switch (rtc_get_reset_reason(0)) {
                case 12 : msg = "System has rebooted due to a core panic."; bigError = true; isError = true; break;
                case 15 : msg = "System has rebooted due to a brownout.<br>Power supply possibly too weak/unstable."; bigError = true; isError = true; break;
                default : msg = ""; isError = false;
            }
        } else {
            bigError = false;
        }
    }

    if (isError) msg = "<span id='error_message' style='color: red'>" + msg + "</span>";

    return msg;
}

#endif