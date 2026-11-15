
// =====================================================
// TRAINING TECH 2026 - DAY 2
// D2-02: KONTROL LED DARI DASHBOARD
// -----------------------------------------------------
// Lanjutan D2-01. Yang BARU di tahap ini:
//   + LED di pin 18 (sama seperti Day 1)
//   + Field "led" pada /api/status
//   + Endpoint POST /api/led  (state = on / off / toggle)
//
// Endpoint:
//   GET  /api/status -> {"online":true,"device":"...","led":false}
//   POST /api/led    -> {"ok":true,"led":true}
//
// Pasangan dashboard: 02_Dashboard/Tahap_2_LED
// =====================================================

#include <WiFi.h>
#include <WebServer.h>

// ======================================
// 1. KONFIGURASI WIFI (HOTSPOT HP)
// ======================================
// Satu kelompok = satu hotspot HP (seperti D1-04B).
// - Beri nama hotspot yang unik, misal "IOT_KELOMPOK_1"
// - Hotspot WAJIB 2,4 GHz (ESP32 tidak bisa 5 GHz)
// - Laptop kelompok juga harus tersambung ke hotspot ini

const char* ssid = "NAMA_HOTSPOT";
const char* password = "PASSWORD_HOTSPOT";

// ======================================
// 2. KONFIGURASI HARDWARE  (BARU)
// ======================================

const char* deviceName = "ESP32 Training Tech 2026";

#define LED_PIN 18

bool ledOn = false;

// ======================================
// 3. MEMBUAT WEBSERVER
// ======================================

WebServer server(80);

bool serverRunning = false;

unsigned long lastWiFiRetry = 0;
const unsigned long retryInterval = 10000;

// ======================================
// 4. HEADER CORS
// ======================================

void kirimHeaderCORS() {

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

void handleOptions() {

  kirimHeaderCORS();
  server.send(204);
}

// ======================================
// 5. HALAMAN UTAMA (CEK CEPAT DI BROWSER)
// ======================================

void handleRoot() {

  String html = R"HTML(
    <!DOCTYPE html>
    <html lang="id">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport"
            content="width=device-width, initial-scale=1">
      <title>ESP32 API</title>
    </head>
    <body>
      <h1>ESP32 API Aktif</h1>
      <p>Buka dashboard di laptop, lalu masukkan IP ESP32 ini.</p>
      <p>Cek data: <a href="/api/status">/api/status</a></p>
    </body>
    </html>
  )HTML";

  server.send(200, "text/html; charset=utf-8", html);

  Serial.println("[HTTP] GET / -> 200 OK");
}

// ======================================
// 6. JSON API STATUS
// ======================================

void handleApiStatus() {

  String json = "{";

  json += "\"online\":true";

  json += ",\"device\":\"";
  json += deviceName;
  json += "\"";

  // Status LED  (BARU)
  json += ",\"led\":";
  json += (ledOn ? "true" : "false");

  json += "}";

  kirimHeaderCORS();

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );

  Serial.println("[HTTP] GET /api/status -> 200 OK");
}

// ======================================
// 7. MENGENDALIKAN LED  (BARU)
// ======================================
// Dashboard mengirim: state=on / state=off / state=toggle
// ESP32 membalas dengan JSON status LED terbaru.
// Bedanya dengan D1-06: tidak lagi redirect ke "/",
// melainkan membalas JSON agar bisa dibaca JavaScript.

void handleLed() {

  String state = server.arg("state");

  if (state == "on") {

    ledOn = true;

  } else if (state == "off") {

    ledOn = false;

  } else if (state == "toggle") {

    ledOn = !ledOn;

  } else {

    kirimHeaderCORS();

    server.send(
      400,
      "application/json; charset=utf-8",
      "{\"ok\":false,\"error\":\"Perintah LED tidak valid\"}"
    );

    return;
  }

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  Serial.print("[LED] ");
  Serial.println(ledOn ? "MENYALA" : "MATI");

  String json = "{\"ok\":true,\"led\":";
  json += (ledOn ? "true" : "false");
  json += "}";

  kirimHeaderCORS();

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}

// ======================================
// 8. HALAMAN TIDAK DITEMUKAN
// ======================================

void handleNotFound() {

  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }

  Serial.print("[HTTP] 404: ");
  Serial.println(server.uri());

  kirimHeaderCORS();

  server.send(
    404,
    "text/plain",
    "404 - Halaman tidak ditemukan"
  );
}

// ======================================
// 9. MEMULAI KONEKSI WIFI
// ======================================

void mulaiWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  Serial.print("Menghubungkan ke hotspot: ");
  Serial.println(ssid);

  // Koneksi WPA2-Personal (sama seperti D1-04B)
  WiFi.begin(ssid, password);

  lastWiFiRetry = millis();
}

// ======================================
// 10. MENGELOLA KONEKSI WIFI
// ======================================

void manageWiFi() {

  if (WiFi.status() == WL_CONNECTED) {

    if (!serverRunning) {

      server.begin();
      serverRunning = true;

      Serial.println();
      Serial.println("[WiFi] TERHUBUNG!");

      Serial.print("IP ESP32: ");
      Serial.println(WiFi.localIP());

      Serial.println("[HTTP] WebServer aktif");
      Serial.println(">> Ketik IP di atas pada dashboard.");
    }

    server.handleClient();
    return;
  }

  if (serverRunning) {

    server.stop();
    serverRunning = false;

    Serial.println("[WiFi] Koneksi terputus");
  }

  if (millis() - lastWiFiRetry >= retryInterval) {

    lastWiFiRetry = millis();

    Serial.println("[WiFi] Mencoba reconnect...");

    WiFi.reconnect();
  }
}

// ======================================
// 11. SETUP
// ======================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("TRAINING TECH 2026");
  Serial.println("D2-02: KONTROL LED DARI DASHBOARD");

  // Konfigurasi LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledOn = false;

  mulaiWiFi();

  // Mendaftarkan route
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);

  server.on("/api/led", HTTP_POST, handleLed);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);

  server.onNotFound(handleNotFound);

  Serial.println("Menunggu koneksi Wi-Fi...");
}

// ======================================
// 12. LOOP
// ======================================

void loop() {

  manageWiFi();

  delay(2);
}
