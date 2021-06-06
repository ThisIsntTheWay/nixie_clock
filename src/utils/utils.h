#include <utils/network.h>
#include "rom/rtc.h"

#ifndef utils_h
#define utils_h

void getESPresetReason(RESET_REASON reason)
{

}

String getSysMsg() {
    String msg;
    bool isError = false;

    if (APisFallback) {
        msg = "Could not connect to WiFi betwork '" + parseNetConfig(4) + "'."; isError = true;
    } else {
        switch (rtc_get_reset_reason(0)) {
            case 15 : msg = "Brownout detection triggered! Power supply possibly unstable."; isError = true; break;/**<15, Reset when the vdd voltage is not stable*/
            default : msg = "No reason";
        }
    }

    if (isError) msg = "<span style='color: red'>" + msg + "</span>";
    return msg;
}

#endif utils_h
