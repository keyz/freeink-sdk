#pragma once

// FreeInk SDK — drop-in HTTPS client shim over SecureClient (TLS 1.3).
//
// Mirrors the slice of Arduino HTTPClient that firmware typically uses, but
// runs requests over a wolfSSL TLS-1.3 transport instead of system mbedTLS, so
// existing call sites (e.g. CrossPoint's HttpDownloader / KOReaderSyncClient)
// switch to the TLS-1.3 path with minimal churn:
//
//   SecureHttpClient http;
//   http.setCACert(rootPem);            // or http.setInsecure();
//   if (http.begin("https://host/path")) {
//     int code = http.GET();
//     String body = http.getString();
//     http.end();
//   }
//
// Header-only; built around Arduino HTTPClient + freeink::SecureClient.

#include <Arduino.h>
#include <HTTPClient.h>

#include "SecureClient.h"

namespace freeink {

class SecureHttpClient {
 public:
  void setCACert(const char* rootCA) { _client.setCACert(rootCA); }
  void setInsecure() { _client.setInsecure(); }

  bool begin(const String& url) { return _http.begin(_client, url); }
  void end() { _http.end(); }

  void addHeader(const String& name, const String& value) { _http.addHeader(name, value); }
  void setTimeout(uint16_t ms) { _http.setTimeout(ms); }
  void setReuse(bool reuse) { _http.setReuse(reuse); }

  int GET() { return _http.GET(); }
  int POST(const String& payload) { return _http.POST(payload); }
  int POST(uint8_t* payload, size_t size) { return _http.POST(payload, size); }
  int sendRequest(const char* method, const String& payload) { return _http.sendRequest(method, payload); }

  String getString() { return _http.getString(); }
  int getSize() { return _http.getSize(); }
  WiFiClient& getStream() { return _http.getStream(); }
  String header(const char* name) { return _http.header(name); }

  // True when the underlying transport actually supports TLS 1.3.
  static bool tls13Available() { return SecureClient::tls13Available(); }

 private:
  SecureClient _client;
  HTTPClient _http;
};

}  // namespace freeink
