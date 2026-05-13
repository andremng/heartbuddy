// =============================================================================
// network.ino
// Handles WiFi connection, NTP time synchronisation, and HTTP POST transmission
// to the HeartBuddy API.
// =============================================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// -----------------------------------------------------------------------------
// CONFIGURATION
// -----------------------------------------------------------------------------

const char* WIFI_SSID     = "Xiaomi 15";
const char* WIFI_PASSWORD = "Kalo2005";
const char* API_URL       = "http://10.59.42.211:5259/api/Readings";

const int SEND_INTERVAL   = 10000;

const char* NTP_SERVER      = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = 3600;
const int   DAYLIGHT_OFFSET = 3600;

unsigned long lastSendTime = 0;
bool wifiConnected = false;
bool timeSync      = false;

// -----------------------------------------------------------------------------
// getFormattedTimestamp()
// Returns current local time as ISO 8601 string: "YYYY-MM-DDTHH:MM:SS"
// Returns empty string if clock is not synchronised.
// -----------------------------------------------------------------------------
String getFormattedTimestamp() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Time: clock not synchronised yet.");
    return "";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  return String(buffer);
}

// -----------------------------------------------------------------------------
// initNetwork()
// Called once from setup(). Connects to WiFi then synchronises clock via NTP.
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
  Serial.print("WiFi connected. IP address of this ESP32: ");
  Serial.println(WiFi.localIP());

  // Print the API URL we will be posting to, useful for debugging.
  Serial.print("Target API URL: ");
  Serial.println(API_URL);

  Serial.println("Synchronising time via NTP...");
  display.clearDisplay();
  display.setCursor(0, 8);
  display.print("Syncing time...");
  display.display();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);

  struct tm timeInfo;
  int syncAttempts = 0;
  while (!getLocalTime(&timeInfo)) {
    delay(500);
    Serial.print(".");
    syncAttempts++;
    if (syncAttempts > 20) {
      Serial.println("\nNTP sync failed. Timestamps will be null.");
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
  Serial.print("Time synchronised. Current time: ");
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
//
// FIX 1 — SpO2 handling:
// spo2Valid is no longer a gate for sending. Instead, when SpO2 is not valid,
// the field is sent as null so the database receives the record regardless.
// Confirm with the backend team that the spo2 column is nullable in SQL Server.
// -----------------------------------------------------------------------------
String buildPayload() {
  String payload = "{";

  payload += "\"deviceID\":1,";
  payload += "\"heartRate\":" + String(beatAvg) + ",";

  // Send the numeric SpO2 value if valid, otherwise send null.
  // null is the correct JSON representation of a missing value,
  // and SQL Server will store it as NULL in a nullable integer column.
  if (spo2Valid) {
    payload += "\"spo2\":" + String(spo2Value) + ",";
  } else {
    payload += "\"spo2\":null,";
  }

  payload += "\"movement\":0,";

  if (timeSync) {
    payload += "\"timestamp\":\"" + getFormattedTimestamp() + "\"";
  } else {
    payload += "\"timestamp\":null";
  }

  payload += "}";
  return payload;
}

// -----------------------------------------------------------------------------
// sendReadingIfNeeded()
// Called every loop() iteration from main.ino.
//
// FIX 1 — SpO2 no longer blocks the send.
//   The only hard requirement before sending is bufferFilled (valid BPM).
//   SpO2 is handled as nullable inside buildPayload().
//
// FIX 2 — Connection refused diagnosis:
//   Added a WiFi status check before every send attempt. If the connection
//   has dropped (common with mobile hotspots), it logs a clear message
//   instead of attempting an HTTP request that will always fail.
//   Also added explicit logging of the full payload and target URL so you
//   can verify both in the Serial Monitor during testing.
// -----------------------------------------------------------------------------
void sendReadingIfNeeded() {
  if (!wifiConnected) return;
  if (millis() - lastSendTime < SEND_INTERVAL) return;

  // FIX 2: Check that WiFi is still connected before attempting any HTTP call.
  // Mobile hotspot IPs can change or the connection can drop silently.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Network: WiFi disconnected. Skipping send.");
    Serial.println("If this persists, restart the device or check hotspot IP.");
    lastSendTime = millis();
    return;
  }

  // FIX 1: Only require BPM to be valid. SpO2 is sent as null if unavailable.
  if (!bufferFilled) {
    Serial.println("Network: skipping send — BPM not yet calibrated.");
    lastSendTime = millis();
    return;
  }

  String payload = buildPayload();

  // Print both the target URL and the payload before sending.
  // This lets you verify in the Serial Monitor that both are correct
  // before suspecting a network problem.
  Serial.println("--- Attempting to send ---");
  Serial.print("URL:     ");
  Serial.println(API_URL);
  Serial.print("Payload: ");
  Serial.println(payload);

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  // Set a connection timeout of 5 seconds.
  // Without this, a refused connection can block the loop for up to 30 seconds.
  http.setTimeout(15000);

  int responseCode = http.POST(payload);

  if (responseCode > 0) {
    Serial.print("HTTP response: ");
    Serial.println(responseCode);
    if (responseCode == 200 || responseCode == 201) {
      Serial.println("Reading sent successfully.");
    } else if (responseCode == 400) {
      Serial.println("Bad request (400). Check field names and data types in buildPayload().");
    } else if (responseCode == 404) {
      Serial.println("Not found (404). Check the API URL path.");
    } else if (responseCode == 500) {
      Serial.println("Server error (500). Ask the backend team to check their logs.");
    } else {
      Serial.print("Unexpected response code: ");
      Serial.println(responseCode);
    }
  } else {
    // Negative codes are ESP32 internal errors, not HTTP responses.
    Serial.print("HTTP request failed. Error: ");
    Serial.println(http.errorToString(responseCode));

    // Specific guidance for the most common failure mode in this setup.
    if (responseCode == -1) {
      Serial.println("Connection refused. Possible causes:");
      Serial.println("  1. API server is not running on the backend machine.");
      Serial.println("  2. IP address or port has changed — confirm with backend team.");
      Serial.println("  3. Firewall is blocking port 5259 on the server machine.");
    }
  }

  http.end();
  lastSendTime = millis();
}