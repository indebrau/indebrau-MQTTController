#ifndef UTIL_HELPERS
#define UTIL_HELPERS

void handleRoot();
void handleWebRequests();
bool loadFromLittleFS(String);
void mqttreconnect();
void mqttcallback(char *, byte *, unsigned int);

#endif