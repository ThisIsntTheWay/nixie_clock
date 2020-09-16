// Webserver implementation

#include "web.h"

// Declare server object and ini on TCP/80
WebServer server(80);

// ================================
// === HTML pages
// ================================

String htmlRoot = "";
String htmlOTA = "";
String htmlSettings = "";

// ================================
// === Web handlers
// ================================

void handle_root() {
  server.send(200, "text/html", htmlRoot);
}

void handle_OTA() {
  server.send(200, "text/html", htmlOTA);
}

void handle_Settings() {
  server.send(200, "text/html", htmlSettings);
}
