// =====================================================
// TRAINING TECH 2026 - DAY 2
// script.js  |  DASHBOARD IOT (VERSI LENGKAP)
// -----------------------------------------------------
// File ini melengkapi index.html dan style.css TANPA
// mengubah keduanya. JavaScript bertugas:
//   1. Menghubungi ESP32 lewat Wi-Fi (HTTP + JSON)
//   2. Menampilkan data ESP32 ke elemen-elemen dashboard
//   3. Mengirim perintah LED ke ESP32
//
// Satu file ini dipakai untuk SEMUA tahap Day 2.
// Kartu yang datanya belum dikirim ESP32 tetap "--":
//   D2_01 -> status koneksi
//   D2_02 -> + LED
//   D2_03 -> + suhu
//   D2_04 -> + kelembapan
//   D2_05 -> + uptime (lengkap)
// =====================================================


// ======================================
// 1. PENGATURAN
// ======================================

// Seberapa sering dashboard meminta data ke ESP32 (ms).
// Disamakan dengan sensorInterval di ESP32 (2500 ms).
const INTERVAL_POLLING = 2500;

// Batas waktu menunggu jawaban ESP32 (ms).
const BATAS_WAKTU = 2000;

// Berapa kali gagal berturut-turut sebelum dianggap terputus.
const BATAS_GAGAL = 3;


// ======================================
// 2. MENGAMBIL ELEMEN HTML
// ======================================
// Setiap elemen diambil berdasarkan id-nya di index.html.

// Koneksi
const statusKoneksi = document.getElementById("statusKoneksi");
const teksKoneksi = document.getElementById("teksKoneksi");
const inputIP = document.getElementById("inputIP");
const tombolHubung = document.getElementById("tombolHubung");
const teksPetunjuk = document.querySelector(".petunjuk");

// Kartu LED
const indikatorLED = document.getElementById("indikatorLED");
const teksLED = document.getElementById("teksLED");
const tombolOn = document.getElementById("tombolOn");
const tombolOff = document.getElementById("tombolOff");
const tombolToggle = document.getElementById("tombolToggle");

// Kartu suhu
const nilaiSuhu = document.getElementById("nilaiSuhu");

// Kartu kelembapan
const nilaiKelembapan = document.getElementById("nilaiKelembapan");

// Informasi sistem
const nilaiUptime = document.getElementById("nilaiUptime");
const waktuUpdate = document.getElementById("waktuUpdate");
const waktuUpdateFooter = document.getElementById("waktuUpdateFooter");


// ======================================
// 3. VARIABEL KONDISI
// ======================================

let alamatESP = "";        // contoh: "http://192.168.43.120"
let terhubung = false;     // true setelah tombol Hubungkan berhasil
let timerPolling = null;   // penyimpan setInterval
let jumlahGagal = 0;       // hitungan gagal berturut-turut
let sedangMengambil = false;

// Teks petunjuk asli (dikembalikan saat koneksi diputus)
const petunjukAsli = teksPetunjuk.textContent;


// ======================================
// 4. FUNGSI BANTU: MENGAMBIL JSON DARI ESP32
// ======================================
// fetch() = cara JavaScript "mengetuk pintu" ESP32.
// AbortController dipakai agar tidak menunggu selamanya.

async function ambilJSON(path, opsi = {}) {

  const pengendali = new AbortController();
  const timer = setTimeout(() => pengendali.abort(), BATAS_WAKTU);

  try {

    const respon = await fetch(alamatESP + path, {
      ...opsi,
      cache: "no-store",
      signal: pengendali.signal
    });

    if (!respon.ok) {
      throw new Error("HTTP " + respon.status);
    }

    // Ubah teks JSON menjadi objek JavaScript
    return await respon.json();

  } finally {

    clearTimeout(timer);
  }
}


// ======================================
// 5. FUNGSI BANTU: IP, PESAN, STATUS
// ======================================

// Membersihkan input, misal "http://192.168.1.5/" -> "192.168.1.5"
function rapikanIP(teks) {

  return teks
    .trim()
    .replace(/^https?:\/\//i, "")
    .replace(/\/+$/, "");
}

// Mengecek format IPv4 (boleh diberi port, misal 192.168.1.5:80)
function ipValid(ip) {

  const cocok = ip.match(/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})(:\d{1,5})?$/);

  if (!cocok) {
    return false;
  }

  return cocok.slice(1, 5).every((angka) => Number(angka) <= 255);
}

// Menampilkan pesan di bawah kolom IP
function tampilkanPesan(teks, jenis = "info") {

  teksPetunjuk.textContent = teks;

  if (jenis === "sukses") {
    teksPetunjuk.style.color = "var(--hijau)";
  } else if (jenis === "error") {
    teksPetunjuk.style.color = "var(--merah)";
  } else {
    teksPetunjuk.style.color = "";
  }
}

// Mengubah badge status di header
function tampilkanKoneksi(ok, teks) {

  statusKoneksi.classList.toggle("terhubung", ok);
  teksKoneksi.textContent = teks || (ok ? "Terhubung" : "Terputus");
}

// Mengubah tulisan tombol Hubungkan / Putuskan
function perbaruiTombolHubung() {

  if (terhubung) {
    tombolHubung.innerHTML = '<i class="bi bi-x-circle-fill"></i> Putuskan';
  } else {
    tombolHubung.innerHTML = '<i class="bi bi-plug-fill"></i> Hubungkan';
  }
}

// Menyimpan IP terakhir agar tidak perlu diketik ulang
function simpanIP(ip) {

  try {
    localStorage.setItem("ipESP32", ip);
  } catch (e) {
    // Abaikan jika browser tidak mengizinkan penyimpanan
  }
}

function muatIP() {

  try {
    return localStorage.getItem("ipESP32") || "";
  } catch (e) {
    return "";
  }
}


// ======================================
// 6. MENAMPILKAN DATA KE DASHBOARD
// ======================================
// Dipanggil setiap kali data baru dari ESP32 diterima.

function perbaruiTampilan(data) {

  // Lihat data mentah di Console (F12 -> Console)
  console.log("Data ESP32:", data);

  // LED (tidak ada = program ESP32 belum mendukung LED)
  tampilkanLED(data.led === true);

  // Suhu (null = sensor gagal dibaca,
  //       tidak ada = program ESP32 belum mengirim suhu)
  if (data.sensor_ok && typeof data.temperature === "number") {
    nilaiSuhu.textContent = data.temperature.toFixed(1);
  } else {
    nilaiSuhu.textContent = "--";
  }

  // Kelembapan
  if (data.sensor_ok && typeof data.humidity === "number") {
    nilaiKelembapan.textContent = data.humidity.toFixed(1);
  } else {
    nilaiKelembapan.textContent = "--";
  }

  // Uptime ESP32
  nilaiUptime.textContent = formatUptime(data.uptime_ms);

  // Waktu pembaruan terakhir (jam laptop)
  const jam = new Date().toLocaleTimeString("id-ID");
  waktuUpdate.textContent = jam;
  waktuUpdateFooter.textContent = jam;
}

// Mengosongkan tampilan saat koneksi diputus
function kosongkanTampilan() {

  tampilkanLED(false);
  nilaiSuhu.textContent = "--";
  nilaiKelembapan.textContent = "--";
  nilaiUptime.textContent = "--";
  waktuUpdate.textContent = "--";
  waktuUpdateFooter.textContent = "-";
}


// ======================================
// 7. FORMAT UPTIME
// ======================================
// 3725000 ms -> "01:02:05"  (jam:menit:detik)
// Lebih dari sehari -> "1 hari 01:02:05"

function formatUptime(ms) {

  if (typeof ms !== "number") {
    return "--";
  }

  const totalDetik = Math.floor(ms / 1000);

  const hari = Math.floor(totalDetik / 86400);
  const jam = Math.floor((totalDetik % 86400) / 3600);
  const menit = Math.floor((totalDetik % 3600) / 60);
  const detik = totalDetik % 60;

  const dua = (n) => String(n).padStart(2, "0");

  const teks = dua(jam) + ":" + dua(menit) + ":" + dua(detik);

  return hari > 0 ? hari + " hari " + teks : teks;
}


// ======================================
// 8. MENGHUBUNGKAN KE ESP32
// ======================================

async function hubungkan() {

  // Jika sudah terhubung, tombol berfungsi sebagai "Putuskan"
  if (terhubung) {
    putuskan();
    return;
  }

  const ip = rapikanIP(inputIP.value);

  if (!ipValid(ip)) {
    tampilkanPesan("Format IP belum benar. Contoh: 192.168.43.120", "error");
    inputIP.focus();
    return;
  }

  alamatESP = "http://" + ip;

  tombolHubung.disabled = true;
  tampilkanKoneksi(false, "Menghubungkan...");
  tampilkanPesan("Menghubungi " + alamatESP + " ...");

  try {

    // Percobaan pertama: minta status ke ESP32
    const data = await ambilJSON("/api/status");

    terhubung = true;
    jumlahGagal = 0;

    tampilkanKoneksi(true);
    tampilkanPesan("Berhasil terhubung ke " + (data.device || "ESP32") + " (" + ip + ").", "sukses");

    simpanIP(ip);
    perbaruiTampilan(data);
    aturTombolLED(true);

    mulaiPolling();

  } catch (err) {

    console.error("Gagal terhubung:", err);

    tampilkanKoneksi(false);
    tampilkanPesan("Gagal terhubung. Cek IP, pastikan laptop & ESP32 di Wi-Fi yang sama, dan ESP32 menyala.", "error");

  } finally {

    tombolHubung.disabled = false;
    perbaruiTombolHubung();
  }
}

function putuskan() {

  hentikanPolling();

  terhubung = false;
  jumlahGagal = 0;

  tampilkanKoneksi(false);
  tampilkanPesan(petunjukAsli);
  kosongkanTampilan();
  aturTombolLED(false);
  perbaruiTombolHubung();
}


// ======================================
// 9. POLLING: MEMINTA DATA SECARA BERKALA
// ======================================
// Setiap INTERVAL_POLLING ms, dashboard bertanya lagi ke ESP32.

function mulaiPolling() {

  hentikanPolling();
  timerPolling = setInterval(ambilStatus, INTERVAL_POLLING);
}

function hentikanPolling() {

  if (timerPolling !== null) {
    clearInterval(timerPolling);
    timerPolling = null;
  }
}

async function ambilStatus() {

  // Jangan kirim permintaan baru jika yang lama belum selesai
  if (sedangMengambil) {
    return;
  }

  sedangMengambil = true;

  try {

    const data = await ambilJSON("/api/status");

    // Jika sebelumnya sempat putus, tandai tersambung lagi
    if (jumlahGagal >= BATAS_GAGAL) {
      tampilkanPesan("Koneksi ke ESP32 pulih kembali.", "sukses");
    }

    jumlahGagal = 0;
    tampilkanKoneksi(true);
    perbaruiTampilan(data);

  } catch (err) {

    jumlahGagal++;

    console.warn("Polling gagal (" + jumlahGagal + "x):", err.message);

    if (jumlahGagal === BATAS_GAGAL) {
      tampilkanKoneksi(false);
      tampilkanPesan("Koneksi ke ESP32 hilang. Dashboard terus mencoba otomatis...", "error");
    }

  } finally {

    sedangMengambil = false;
  }
}


// ======================================
// 10. KONTROL LED
// ======================================

// Mengubah tampilan lampu di kartu LED
function tampilkanLED(nyala) {

  indikatorLED.classList.toggle("nyala", nyala === true);
  teksLED.textContent = nyala ? "ON" : "OFF";
}

// Mengaktifkan / menonaktifkan tombol LED
function aturTombolLED(aktif) {

  tombolOn.disabled = !aktif;
  tombolOff.disabled = !aktif;
  tombolToggle.disabled = !aktif;
}

// Mengirim perintah ke ESP32: "on", "off", atau "toggle"
async function kirimPerintahLED(state) {

  if (!terhubung) {
    tampilkanPesan("Hubungkan ke ESP32 terlebih dahulu.", "error");
    return;
  }

  aturTombolLED(false);

  try {

    // Dikirim seperti form HTML: state=on
    const data = await ambilJSON("/api/led", {
      method: "POST",
      body: new URLSearchParams({ state: state })
    });

    tampilkanLED(data.led);

  } catch (err) {

    console.error("Perintah LED gagal:", err);

    // 404 = program ESP32 belum punya /api/led (masih D2_01)
    if (err.message === "HTTP 404") {
      tampilkanPesan("ESP32 belum mendukung kontrol LED. Upload program D2_02 atau sesudahnya.", "error");
    } else {
      tampilkanPesan("Perintah LED gagal dikirim. Coba lagi.", "error");
    }

  } finally {

    aturTombolLED(true);
  }
}


// ======================================
// 11. MENGHUBUNGKAN TOMBOL DENGAN FUNGSI
// ======================================

tombolHubung.addEventListener("click", hubungkan);

// Tekan Enter di kolom IP = klik Hubungkan
inputIP.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    hubungkan();
  }
});

tombolOn.addEventListener("click", () => kirimPerintahLED("on"));
tombolOff.addEventListener("click", () => kirimPerintahLED("off"));
tombolToggle.addEventListener("click", () => kirimPerintahLED("toggle"));


// ======================================
// 12. KONDISI AWAL SAAT HALAMAN DIBUKA
// ======================================

inputIP.value = muatIP();
tampilkanKoneksi(false);
aturTombolLED(false);
