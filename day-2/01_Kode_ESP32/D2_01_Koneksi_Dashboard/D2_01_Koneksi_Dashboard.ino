
// =====================================================
// TRAINING TECH 2026 - DAY 2
// D2-01: KONEKSI ESP32 KE DASHBOARD
// -----------------------------------------------------
// Tujuan tahap ini:
//   Dashboard (website di laptop) bisa "menyapa" ESP32
//   dan mendapat balasan. Jika berhasil, status di
//   pojok kanan atas dashboard berubah menjadi
//   "Terhubung".
//
// Endpoint (alamat API) yang tersedia:
//   GET /api/status  -> {"online":true,"device":"..."}
//
// Pasangan dashboard: 02_Dashboard/Tahap_1_Koneksi
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
// 2. IDENTITAS PERANGKAT
// ======================================

const char* deviceName = "ESP32 Training Tech 2026";

// ======================================
// 3. MEMBUAT WEBSERVER
// ======================================

WebServer server(80);

bool serverRunning = false;

unsigned long lastWiFiRetry = 0;
const unsigned long retryInterval = 10000;

// ======================================
// 4. HEADER CORS  (BARU DI DAY 2)
// ======================================
// Dashboard dibuka dari laptop (misal http://127.0.0.1:5500),
// sedangkan ESP32 punya alamat lain (misal http://192.168.1.20).
// Karena "asal"-nya berbeda, browser akan MEMBLOKIR data
// kecuali ESP32 memberi izin lewat header CORS berikut.

void kirimHeaderCORS() {

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

// Browser kadang "bertanya dulu" (request OPTIONS)
// sebelum mengirim data. Cukup dijawab dengan izin CORS.
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
// Inilah "jawaban" ESP32 ketika dashboard bertanya.
// Formatnya JSON: pasangan "nama":nilai.

void handleApiStatus() {

  String json = "{";

  json += "\"online\":true";

  json += ",\"device\":\"";
  json += deviceName;
  json += "\"";

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
// 7. HALAMAN TIDAK DITEMUKAN
// ======================================

void handleNotFound() {

  // Jawab semua pertanyaan OPTIONS dengan izin CORS
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
// 8. MEMULAI KONEKSI WIFI
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
// 9. MENGELOLA KONEKSI WIFI
// ======================================
// Sama seperti D1-05 s.d. D1-07.

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
// 10. SETUP
// ======================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("TRAINING TECH 2026");
  Serial.println("D2-01: KONEKSI ESP32 KE DASHBOARD");

  mulaiWiFi();

  // Mendaftarkan route
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);

  server.onNotFound(handleNotFound);

  Serial.println("Menunggu koneksi Wi-Fi...");
}

// ======================================
// 11. LOOP
// ======================================

void loop() {

  manageWiFi();

  delay(2);
}
