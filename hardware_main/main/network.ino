// =============================================================================
// network.ino
// Handles WiFi connection, NTP time synchronisation, and HTTP POST transmission
// to the HeartBuddy API.
// =============================================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>  // Standard C library for time functions, included with ESP32 core.

// -----------------------------------------------------------------------------
// CONFIGURATION
// -----------------------------------------------------------------------------

const char* WIFI_SSID     = "YOUR_NETWORK_NAME";
const char* WIFI_PASSWORD = "YOUR_NETWORK_PASSWORD";
const char* API_URL       = "http://ASK_BACKEND_TEAM/api/readings";
const char* DEVICE_ID     = "esp32-001";

const int SEND_INTERVAL   = 10000;

// NTP server to request the current time from.
// pool.ntp.org is a public server that is reliable and free to use.
// The second and third parameters are UTC offset in seconds and
// daylight saving offset in seconds.
// For Netherlands (CET = UTC+1): gmtOffset = 3600, daylightOffset = 3600.
// Adjust these values if the device will be used in a different timezone.
const char* NTP_SERVER       = "pool.ntp.org";
const long  GMT_OFFSET_SEC   = 3600;   // UTC+1 for Central European Time
const int   DAYLIGHT_OFFSET  = 3600;   // +1 hour for Central European Summer Time

unsigned long lastSendTime = 0;
bool wifiConnected  = false;
bool timeSync       = false; // Tracks whether NTP sync was successful.

// -----------------------------------------------------------------------------
// getFormattedTimestamp()
// Reads the current time from the ESP32 internal clock (synchronised via NTP)
// and returns it as an ISO 8601 string: "YYYY-MM-DDTHH:MM:SS"
// This format is the international standard for timestamps and is natively
// understood by C#, SQL Server, and JavaScript.
// Returns an empty string if the clock has not been synchronised yet.
// -----------------------------------------------------------------------------
String getFormattedTimestamp() {
  struct tm timeInfo; // tm is a C struct that holds broken-down time components.

  // getLocalTime() fills the timeInfo struct with the current local time.
  // The second parameter is the maximum milliseconds to wait for a valid time.
  // Returns false if the clock is not synchronised.
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Time: clock not synchronised yet.");
    return "";
  }

  // strftime formats the time struct into a string using format codes.
  // %Y = 4-digit year, %m = month, %d = day,
  // %H = hour (24h), %M = minute, %S = second.
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  return String(buffer);
}

// -----------------------------------------------------------------------------
// initNetwork()
// Called once from setup() in main.ino.
// Connects to WiFi, then synchronises the clock via NTP.
// -----------------------------------------------------------------------------
void initNetwork() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 8);
  display.print("Connecting WiFi...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;

    if (attempts > 30) {
      Serial.println("\nWiFi connection failed. Running in offline mode.");
      display.clearDisplay();
      display.setCursor(0, 4);
      display.print("WiFi failed.");
      display.setCursor(0, 18);
      display.print("Offline mode.");
      display.display();
      delay(2000);
      return;
    }
  }

  wifiConnected = true;
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // --- NTP SYNCHRONISATION ---
  // configTime sends a request to the NTP server and sets the ESP32 internal
  // clock. This only needs to happen once; the clock runs autonomously after.
  Serial.println("Synchronising time via NTP...");

  display.clearDisplay();
  display.setCursor(0, 8);
  display.print("Syncing time...");
  display.display();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);

  // Wait up to 10 seconds for the time to synchronise before continuing.
  struct tm timeInfo;
  int syncAttempts = 0;
  while (!getLocalTime(&timeInfo)) {
    delay(500);
    Serial.print(".");
    syncAttempts++;

    if (syncAttempts > 20) {
      Serial.println("\nNTP sync failed. Timestamps will be omitted.");
      // timeSync remains false; buildPayload() will handle the missing timestamp.
      display.clearDisplay();
      display.setCursor(0, 4);
      display.print("Time sync failed.");
      display.setCursor(0, 18);
      display.print("No timestamps.");
      display.display();
      delay(2000);
      return;
    }
  }

  timeSync = true;
  Serial.println();
  Serial.println("Time synchronised successfully.");

  // Print the synchronised time to Serial for confirmation.
  Serial.print("Current time: ");
  Serial.println(getFormattedTimestamp());

  display.clearDisplay();
  display.setCursor(0, 4);
  display.print("WiFi + Time OK");
  display.setCursor(0, 18);
  display.print(WiFi.localIP());
  display.display();
  delay(2000);
}

// -----------------------------------------------------------------------------
// buildPayload()
// Constructs the JSON string sent to the API.
// Includes the timestamp only if NTP synchronisation was successful.
// If the timestamp field is absent, the backend can fall back to server time.
// -----------------------------------------------------------------------------
String buildPayload() {
  String payload = "{";
  payload += "\"deviceId\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"bpm\":"        + String(beatAvg)    + ",";
  payload += "\"spo2\":"       + String(spo2Value)  + ",";
  payload += "\"movementDetected\":false,";

  // Only include the timestamp field if the clock is synchronised.
  // If NTP failed, the field is omitted entirely and the API server
  // should fall back to generating the timestamp on its end.
  if (timeSync) {
    String ts = getFormattedTimestamp();
    payload += "\"timestamp\":\"" + ts + "\"";
  } else {
    // Send a null value so the field is present but explicitly empty.
    // This tells the backend that the device is online but has no clock.
    payload += "\"timestamp\":null";
  }

  payload += "}";
  return payload;
}

// -----------------------------------------------------------------------------
// sendReadingIfNeeded()
// Called every loop() iteration from main.ino.
// Identical logic to the previous version, with timestamp now included
// in the payload via buildPayload().
// -----------------------------------------------------------------------------
void sendReadingIfNeeded() {
  if (!wifiConnected) return;
  if (millis() - lastSendTime < SEND_INTERVAL) return;

  if (!bufferFilled || !spo2Valid) {
    Serial.println("Network: skipping send — readings not yet valid.");
    lastSendTime = millis();
    return;
  }

  String payload = buildPayload();
  Serial.print("Sending payload: ");
  Serial.println(payload);

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  int responseCode = http.POST(payload);

  if (responseCode > 0) {
    Serial.print("HTTP response: ");
    Serial.println(responseCode);
    if (responseCode == 200 || responseCode == 201) {
      Serial.println("Reading sent successfully.");
    } else {
      Serial.println("Unexpected response. Check API URL and payload field names.");
    }
  } else {
    Serial.print("HTTP request failed. Error: ");
    Serial.println(http.errorToString(responseCode));
  }

  http.end();
  lastSendTime = millis();
}