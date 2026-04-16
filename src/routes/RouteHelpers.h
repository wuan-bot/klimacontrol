#ifndef KLIMACONTROL_ROUTEHELPERS_H
#define KLIMACONTROL_ROUTEHELPERS_H

#ifdef ARDUINO
#include <ESPAsyncWebServer.h>

// Content type constants
inline const char* CONTENT_TYPE_HTML = "text/html";
inline const char* CONTENT_TYPE_CSS = "text/css";
inline const char* CONTENT_TYPE_SVG = "image/svg+xml";
inline const char* CONTENT_TYPE_JSON = "application/json";

// JSON Key constants
inline const char* JSON_KEY_SUCCESS = "success";
inline const char* JSON_KEY_VALUE = "value";
inline const char* JSON_KEY_NAME = "name";

// Common JSON Responses
inline const char* JSON_RESPONSE_SUCCESS = "{\"success\":true}";
inline const char* JSON_RESPONSE_ERROR_INVALID_JSON = R"({"success":false,"error":"Invalid JSON"})";

inline void sendGzippedResponse(AsyncWebServerRequest *request, const char *contentType, const uint8_t *data, size_t len) {
    AsyncWebServerResponse *response = request->beginResponse(200, contentType, data, len);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400");
    request->send(response);
}

#endif

#endif //KLIMACONTROL_ROUTEHELPERS_H
