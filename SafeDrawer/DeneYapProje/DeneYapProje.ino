#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "lsm6dsm.h"

// ================= KULLANICI AYARLARI =================
const char* ssid = "WİFİ_ADİNİZ";            
const char* password = "WİFİ_SİFRENİZ";                   

#define BOT_TOKEN "TELEGRAM_BOT_TOKEN"
#define CHAT_ID "TELEGRAM_CHAT_ID"
// ======================================================

#define BUZZER_PIN D12      // Kamerayla çakışmayan D12 pini
#define HAREKET_ESIGI 0.18  // Çekmece sarsıntısını anında yakalayan hassas eşik

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
LSM6DSM IMU;

bool kameraHazir = false;

void buzzerSesVer() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}

void kameraBaslat() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // Orijinal Deneyap Kamera soket pinleri
  config.pin_d0 = CAMD2;
  config.pin_d1 = CAMD3;
  config.pin_d2 = CAMD4;
  config.pin_d3 = CAMD5;
  config.pin_d4 = CAMD6;
  config.pin_d5 = CAMD7;
  config.pin_d6 = CAMD8;
  config.pin_d7 = CAMD9;
  config.pin_xclk = CAMXC;
  config.pin_pclk = CAMPC;
  config.pin_vsync = CAMV;
  config.pin_href = CAMH;
  config.pin_sscb_sda = CAMSD;
  config.pin_sscb_scl = CAMSC;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA; // 640x480 net görüntü
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera baslatilamadi, Hata kodu: 0x%x\n", err);
    kameraHazir = false;
    return;
  }

  Serial.println("Kamera BASARIYLA baslatildi!");
  kameraHazir = true;
}

void telegramBildirimVeFotoGonder() {
  // 1.Önce hemen çekmecenin açıldığına dair metin bildirimini ilet
  Serial.println("Telegram acil durum metin bildirimi gonderiliyor...");
  bot.sendMessage(CHAT_ID, "🚨 DIKKAT! Cekmeceniz acildi, hareket algilandi!\n📷 Fotograf cekiliyor...", "");

  if (!kameraHazir) {
    bot.sendMessage(CHAT_ID, "⚠️ Kamera donanimi hazir olmadigi icin goruntu iletilemedi.", "");
    return;
  }

  // 2. Sensör tamponundaki eski kareyi temizle (Bayat kareyi at)
  camera_fb_t * eskiFb = esp_camera_fb_get();
  if (eskiFb) {
    esp_camera_fb_return(eskiFb);
  }

  // Anlık taze kareyi yakala
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Kamera karesi alinamadi!");
    bot.sendMessage(CHAT_ID, "⚠️ Fotograf cekimi basarisiz oldu.", "");
    return;
  }

  Serial.println("Fotograf Telegram'a iletiliyor...");

  // 3.Fotoğrafı açıklamasıyla birlikte güvenli HTTPS bağlantısıyla gönder
  if (secured_client.connect("api.telegram.org", 443)) {
    String caption = "🚨 Olay Ani Kanit Fotografi";
    String head = "--SafeDrawerBoundary\r\n"
                  "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + String(CHAT_ID) + "\r\n"
                  "--SafeDrawerBoundary\r\n"
                  "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" + caption + "\r\n"
                  "--SafeDrawerBoundary\r\n"
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"cekmece.jpg\"\r\n"
                  "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--SafeDrawerBoundary--\r\n";

    uint32_t totalLen = fb->len + head.length() + tail.length();

    secured_client.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
    secured_client.println("Host: api.telegram.org");
    secured_client.println("Content-Length: " + String(totalLen));
    secured_client.println("Content-Type: multipart/form-data; boundary=SafeDrawerBoundary");
    secured_client.println();
    secured_client.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) {
        secured_client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        secured_client.write(fbBuf, remainder);
      }
    }

    secured_client.print(tail);
    Serial.println("Fotograf Telegram'a basariyla iletildi!");
  } else {
    Serial.println("Telegram sunucusuna baglanilamadi.");
  }

  esp_camera_fb_return(fb);
  secured_client.stop(); // Baglantiyi temiz bir sekilde kapat
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin();
  delay(100);

  SensorSettings ayarlar;
  ayarlar.accelEnabled = 1;
  ayarlar.accelRange = 2;
  ayarlar.accelSampleRate = 104;
  ayarlar.accelBandWidth = 50;
  ayarlar.accelFifoEnabled = 0;

  IMU.begin(&ayarlar);
  Serial.println("\nLSM6DSM Ivmeolcer Hazir!");

  // Kamerayı başlat
  kameraBaslat();

  // Wi-Fi
  Serial.print("Wi-Fi baglaniyor: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi baglandi!");

  // Açılış testi
  buzzerSesVer();
  bot.sendMessage(CHAT_ID, "✅ SafeDrawer devrede! Cekmece koruma altinda.", "");
}

void loop() {
  float x = IMU.readFloatAccelX();
  float y = IMU.readFloatAccelY();
  float z = IMU.readFloatAccelZ();

  float toplamIvme = sqrt(x * x + y * y + z * z);
  float hareketMiktari = abs(toplamIvme - 1.0);

  // Çekmece açıldığında / hareket algılandığında tetikle
  if (hareketMiktari > HAREKET_ESIGI && toplamIvme > 0.1) {
    Serial.println("\n******************************************");
    Serial.println(">>> CEKMECE ACILDI! HAREKET ALGILANDI <<<");
    Serial.println("******************************************");

    // 1. Buzzer ötsün
    buzzerSesVer();

    // 2. Önce bildirim uyarısı, hemen ardından fotoğraf gitsin
    telegramBildirimVeFotoGonder();

    Serial.println("Sistem hazir duruma geciyor (2.5 sn bekleme)...\n");
    delay(2500);
  }

  delay(50);
}