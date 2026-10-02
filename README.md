# 🗄️ SafeDrawer: Akıllı Çekmece Güvenlik ve Erken Uyarı Sistemi

SafeDrawer, değerli eşyaların veya gizli evrakların saklandığı çekmecelerin yetkisiz şekilde açılmasını anında tespit eden, sesli alarm veren ve olayın kanıt fotoğrafını Telegram üzerinden kullanıcıya ileten IoT tabanlı bir güvenlik prototipidir.

---

## 📌 Özellikler

- **Hassas Hareket Tespiti:** Deneyap Kart üzerindeki dahili LSM6DSM ivmeölçer sayesinde çekmece açılma sarsıntısını anında yakalar.
- **Yerçekimi Filtreleme:** Statik yerçekimi bileşeni elenerek yalnızca dinamik çekmece hareketlerine duyarlı hale getirilmiştir.
- **Lokal Sesli Alarm:** Hareket anında harici buzzer ile sesli caydırıcı alarm tetiklenir.
- **Anlık Kanıt Fotoğrafı:** Hareket algılandığı anda Deneyap Kamera (OV2640) ile olay anı yakalanır.
- **Telegram Entegrasyonu:** Çekmece açıldığında önce acil durum metin bildirimi, hemen ardından çekilen olay anı fotoğrafı HTTPS üzerinden Telegram botuna gönderilir.

---

## 🛠️ Donanım Bileşenleri

- **Ana Kart:** Deneyap Kart (ESP32-WROVER-E)
- **Kamera:** Deneyap Kamera Modülü (OV2640)
- **Sensör:** Dahili LSM6DSM (6 Eksen Ataletsel Ölçüm Birimi)
- **Uyarı:** 5V Buzzer
- **Bağlantı:** FPC Kamera Kablosu, Jumper Kablolar

---

## 🔌 Pin Bağlantı Şeması

| Bileşen | Deneyap Kart Pini | Açıklama |
| :--- | :--- | :--- |
| **Buzzer (+)** | `D12` | Tetikleme pini (Kamera hatlarıyla çakışmaz) |
| **Buzzer (-)** | `GND` | Toprak hattı |
| **LSM6DSM** | Dahili I2C (`0x6A`) | Kart üzerindeki dahili bağlantı |
| **Deneyap Kamera** | Dahili FPC Soketi | Donanımsal kamera portu |

---

## 💻 Kurulum ve Çalıştırma

1. Arduino IDE üzerinden `SafeDrawer.ino` dosyasını açın.
2. Gerekli kütüphaneleri kurun:
   - `UniversalTelegramBot`
   - `ArduinoJson`
   - `lsm6dsm` (Deneyap kütüphanesi)
3. Kod içerisindeki kullanıcı alanlarını kendi bilgilerinizle güncelleyin:
   ```cpp
   const char* ssid = "WIFI_ADINIZ";
   const char* password = "WIFI_SIFRENIZ";
   #define BOT_TOKEN "TELEGRAM_BOT_TOKEN"
   #define CHAT_ID "TELEGRAM_CHAT_ID"
