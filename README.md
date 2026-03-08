# 📻 TA2CAY  İnternet Radyosu Projesi

Bu proje, ESP32 mikrodenetleyici, VS1053 ses dekoderi ve ST7735 TFT ekran kullanılarak yapılmış, modern arayüzlü ve yüksek ses kaliteli bir internet radyosudur.

## 🚀 Özellikler
- **Yüksek Ses Kalitesi**: VS1053 donanımsal MP3 dekoderi ile kesintisiz müzik.
- **Premium Arayüz**: 1.8" TFT ekran üzerinde hava durumu, saat, tarih ve canlı VU metre.
- **Kategorik İstasyonlar**: Pop, Arabesk, Haber ve Türkü kategorilerinde 20+ hazır kanal.
- **Akıllı WiFi**: 3 farklı WiFi ağına otomatik bağlanma desteği.
- **Kolay Kontrol**: KY-040 Rotary Encoder ile hızlı kanal değişimi ve menü gezintisi.

## 🛠 Donanım Bileşenleri
1. **ESP32 DevKit V1**
2. **VS1053B MP3 Shield/Module**
3. **ST7735 1.8" TFT Ekran** (128x160)
4. **KY-040 Rotary Encoder**
5. **Hoparlör & Amfi** (PAM8403 önerilir)

## 🔌 Pin Bağlantıları (Şema)

Projenin en kritik noktası kablolamadır. Aşağıdaki tabloyu takip ederek bağlantıları yapabilirsiniz:

### 1. TFT Ekran (ST7735) Bağlantısı
| TFT Pin | ESP32 Pin | Açıklama |
| :--- | :--- | :--- |
| **VCC** | 3.3V / 5V | Ekran beslemesi |
| **GND** | GND | Toprak |
| **CS** | GPIO 5 | Chip Select |
| **RESET** | GPIO 17 | Reset |
| **DC/RS** | GPIO 2 | Data/Command |
| **SDI/MOSI**| GPIO 23 | SPI Data |
| **SCK** | GPIO 18 | SPI Clock |
| **LED/BL** | GPIO 26 | Arka Işık (Parlaklık kontrolü) |

### 2. VS1053 Ses Modülü Bağlantısı
| VS1053 Pin | ESP32 Pin | Açıklama |
| :--- | :--- | :--- |
| **VCC** | 5V | Modül beslemesi |
| **GND** | GND | Toprak |
| **MISO** | GPIO 19 | SPI Master In Slave Out |
| **MOSI** | GPIO 23 | SPI Master Out Slave In |
| **SCK** | GPIO 18 | SPI Clock |
| **XCS** | GPIO 16 | Control Chip Select |
| **XDCS** | GPIO 21 | Data Chip Select |
| **DREQ** | GPIO 4 | Data Request Breakout |

### 3. Rotary Encoder (KY-040) Bağlantısı
| Encoder Pin| ESP32 Pin | Açıklama |
| :--- | :--- | :--- |
| **CLK** | GPIO 27 | Saat Sinyali |
| **DT** | GPIO 14 | Data Sinyali |
| **SW** | GPIO 22 | Buton Sinyali |
| **VCC** | 3.3V | Besleme |
| **GND** | GND | Toprak |

---

## 💻 Kurulum Adımları

1. **Arduino IDE'yi Hazırlayın**:
   - ESP32 kart desteğini kurun.
   - Gerekli kütüphaneleri "Library Manager" üzerinden yükleyin:
     - `Adafruit GFX Library`
     - `Adafruit ST7735 and ST7789 Library`
     - `ArduinoJson`
     - `VS1053_IDE_Library` (veya benzeri VS1053 kütüphanesi)

2. **Yazılımı Güncelleyin**:
   - `radyo_pro.ino` dosyasını açın.
   - WiFi bilgilerinizi (SSID ve Şifre) `wifiSSID` ve `wifiPASS` dizilerine girin.

3. **Yükleme Yapın**:
   - ESP32 kartınızı seçin ve "Upload" butonuna basın.


