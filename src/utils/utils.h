#include <utils/network.h>
#include <system/rtc.h>
#include "rom/rtc.h"

#ifndef utils_h
#define utils_h

bool globalErrorOverride = false;
bool bigError = false;

String getSysMsg() {
    String msg = "";
    bool isError = false;

    if (APisFallback && !bigError) {
        msg = "Could not connect to WiFi network '" + parseNetConfig(4) + "'."; isError = true;
    } else if (!NTPisValid && !bigError) {
        msg = "NTP server is unresponsive: '" + parseRTCconfig(1) + "'."; isError = true;
    } else {
        if (!globalErrorOverride) {
            switch (rtc_get_reset_reason(0)) {
                case 12 : msg = "The system has rebooted due to a core panic."; bigError = true; isError = true; break;
                case 15 : msg = "The system has rebooted due to a brownout.<br>Power supply possibly too weak or unstable."; bigError = true; isError = true; break;
                default : msg = ""; isError = false;
            }
        } else {
            bigError = false;
        }
    }

    if (isError) msg = "<span id='error_message' style='color: red' onclick='ackError()'>" + msg + "</span>";

    return msg;
}

#endif