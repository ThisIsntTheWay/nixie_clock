#include <utils/network.h>
#include <system/rtc.h>
#include "rom/rtc.h"

#ifndef utils_h
#define utils_h

bool globalErrorOverride = false;

String getSysMsg() {
    String msg = "";
    bool isError = false;

    if (!globalErrorOverride) {
        if (APisFallback) {
            msg = "Could not connect to WiFi network '" + parseNetConfig(4) + "'."; isError = true;
        } else if (!NTPisValid) {
            msg = "NTP server is unresponsive: '" + parseRTCconfig(1) + "'."; isError = true;
        } else {
            switch (rtc_get_reset_reason(0)) {
                case 12 : msg = "System rebooted due to core panic."; isError = true; break;
                case 15 : msg = "Brownout detector triggered; power supply possibly unstable."; isError = true; break;
                default : msg = "";
            }
        }

        if (isError) msg = "<span style='color: red'>" + msg + "</span>";
    }

    return msg;
}

#endif