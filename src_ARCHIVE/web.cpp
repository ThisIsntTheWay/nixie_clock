// Webserver implementation

#include "web.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ================================
// === HTML pages
// ================================

// Load HTML structure into Progmem
extern const char htmlRoot[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
	<head>
		<link href="https://fonts.googleapis.com/css2?family=Roboto&display=swap" rel="stylesheet">
		<style>
			body { font-family: 'Roboto', sans-serif; }
			h4 { padding-bottom: 0%; }
			div { padding-left: 10%; }
		</style>
		<title>Nixie clock - WebGUI</title>
	</head>
	<body>
		<center>
			<h1>EPS32 nixie clock</h1>
		</center>
		<div>
			<h4>The current time is:</h4>
			NTP: %NTPTIME%<br>
			RTC: %RTCTIME%
			<p></p>

			Please specify an option:<br>
			<ul>
				<li style="list-style-type:none">Control</li>
					<ul>
						<li><a href="/setRTC">Set RTC</a></li>
						<li><a href="/setTubes">Set individual tube</a></li>
					</ul>
				<li style="list-style-type:none">ESP32 management</li>
					<ul>
						<li><a href="/setPower">Power management</a></li>
						<--<li><a href="/setAPI">API management</a></li>--!>
						<--<li><a href="/fwOTA">Firmware upgrade</a></li>--!>
					</ul>
			</ul>
		</div>
	</body>
</html>)rawliteral";

extern const char htmlPower[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - Power management</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

extern const char htmlSettings[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - Settings</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

extern const char htmlOTA[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - Firmware management</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

extern const char htmlRTCControl[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - RTC control</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

extern const char htmlTubeControl[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - Tube control</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

extern const char htmlPowerControl[] PROGMEM = R"rawliteral(
<html>
	<head>
		<title>Nixie clock - Power management</title>
	</head>
	<body>
		<center>
			<h1>Under construction</h1>
		</center>
	</body>
</html>)rawliteral";

// ================================
// === Web handlers
// ================================

void handle_root(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlRoot);
}

void handle_OTA(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlOTA);
}

void handle_Settings(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlSettings);
}

void handle_Power(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlPower);
}

void handle_RTCControl(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlRTCControl);
}

void handle_TubeControl(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlTubeControl);
}

void handle_PowerControl(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlPowerControl);
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Resource not found.");
}
