// Webserver implementation

#include "web.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>

// Declare server object and ini on TCP/80
WebServer server(80);

// ================================
// === HTML pages
// ================================

//String htmlRoot = "";
const char index_html[] PROGMEM = R"rawliteral(
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
						<li><a href="/setAPI">API management</a></li>
						<li><a href="/fwOTA">Firmware upgrade</a></li>
					</ul>
			</ul>
		</div>
	</body>
</html>)rawliteral";

String htmlPower = "";
String htmlSettings = "";

// ================================
// === Web handlers
// ================================

void handle_root(AsyncWebServerRequest *request) {
  server.send(200, "text/html", htmlRoot);
}

void handle_OTA(AsyncWebServerRequest *request) {
  server.send(200, "text/html", htmlOTA);
}

void handle_Settings(AsyncWebServerRequest *request) {
  server.send(200, "text/html", htmlSettings);
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
