/*
  PROFESYONEL INTERNET RADYOSU v4.0 (PREMIUM UI)
  ESP32 + VS1053 + ST7735 1.8" TFT + KY-040
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <VS1053.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi Aglari (3 adet, sirayla denenir)
#define WIFI_COUNT 3
const char *wifiSSID[WIFI_COUNT]     = {"KARTAL1",    "KARTAL_2.4GHz", "dfnk20"};
const char *wifiPASS[WIFI_COUNT]     = {"kartal20",   "kartal67",      "20defne20"};
int connectedWifi = -1;  

// Pin Tanımları
#define VS1053_XCS 16
#define VS1053_XDCS 21
#define VS1053_DREQ 4
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 17
#define TFT_BL 26
#define ENC_CLK 27
#define ENC_DT 14
#define ENC_SW 22

VS1053 player(VS1053_XCS, VS1053_XDCS, VS1053_DREQ);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WiFiClient       httpClient;
WiFiClientSecure httpsClient;
WiFiClient*      activeClient = &httpClient;

// Premium Renk Paleti (RGB565 Formati)
#define C_BG      0x0825  // Koyu Laci/Gri Arkaplan
#define C_HEADER  0x10A6  // Header Arkaplani
#define C_FOOTER  0x10A6  // Footer Arkaplani
#define C_TEXT    0xFFFF  // Beyaz Metin
#define C_ACCENT  0x07E0  // Cirtlak Yesil (Vurgu)
#define C_BLUE    0x3B1F  // Acik Mavi / Turkuaz
#define C_WARN    0xFBE0  // Turuncu/Sari (Uyari)
#define C_ERROR   0xF800  // Kirmizi
#define C_GRAY    0x8410  // Gri
#define C_DARK    0x2124  // Koyu Gri Kutu

// Istasyon Listesi (Kategorik Siralama)
#define NSTATIONS 21

const char *hosts[] = {
    // 1. POP / HIT MUZIK
    "officialbestfm.radyotvonline.net", "kralpopwmp.radyotvonline.com",  
    "edge1.radyotvonline.net", "46.20.3.231", "radyo.yayindakiler.com",
    "radyo.yayin.com.tr", "showradyo.radyotvonline.net", "radyo.yayin.com.tr",
    "ritim.80.yayin.com.tr", "radyo1.radyo-dinle.tc",
    // 2. ARABESK / NOSTALJI
    "anadolu.liderhost.com.tr", "stream.radyo45lik.com",        
    // 3. HABER / GENEL
    "ntvrdwmp.radyotvonline.com", "trt.radyotvonline.net", 
    "radyo.yayindakiler.com", "icecast-rian.cdnvideo.ru", 
    "moondigitaledge.radyotvonline.net",
    // 4. TURKU / KULTUR
    "yayin.turkhosted.com", "trt.radyotvonline.net", 
    "radyo.yayindakiler.com", "yayin.turkhosted.com"
};

const char *paths[] = {
    // POP
    "/bestfmofficial", "/kralpopmp3", "/shoutcast/play/alemfm", "/radyovivampeg",
    "/;", "/;", "/showradyoaac", "/;", "/;", "/8138/stream/;",
    // ARABESK
    "/;", "/stream",
    // HABER / GENEL
    "/ntvradyomp3", "/trthaber", "/;", "/voicestm", "/radyotrafikmarmara/playlist.m3u8",
    // TURKU / KULTUR
    "/stream", "/trtfm", "/;", "/4591/stream"
};

const int sPorts[] = {
    // POP
    80, 80, 443, 80, 3000, 5894, 443, 4052, 443, 443,
    // ARABESK
    10886, 4545,
    // HABER / GENEL
    80, 80, 4118, 443, 443,
    // TURKU / KULTUR
    6006, 80, 4174, 4591
};

const bool sUseTLS[] = {
    // POP
    false, false, true, false, false, false, true, false, true, true,
    // ARABESK
    false, false,
    // HABER / GENEL
    false, false, true, true, true,
    // TURKU / KULTUR
    false, false, true, true
};

const char *sName[] = {
    "BEST FM", "KRAL POP", "ALEM FM", "RADYO VIVA", "RADYO EREGLI", 
    "RADYO KULUP", "SHOW RADYO", "RADYO MEGA", "RADYO STAR", "KARADENIZ",
    "GENERAL FM", "RADYO 45LIK",
    "NTV RADYO", "TRT HABER", "RADYO HABER", "SPUTNIK", "RADYO TRAFIK",
    "RADYO EKIN", "TRT FM", "RADYO CAN", "TURKU RADYO"
};

const char *sGenre[] = {
    "Pop Muzik", "Pop Muzik", "Pop Muzik", "Pop Muzik", "Pop Muzik",
    "Pop Muzik", "Pop Muzik", "Pop Muzik", "Pop Muzik", "Pop Muzik",
    "Arabesk", "Nostalji",
    "Haber", "Haber", "Haber", "Haber", "Genel",
    "Turku", "Kultur", "Turku", "Turku"
};

int curStation = 0;
int curVolume = 100; 
bool isPlaying = false; 

// --- v5.0 RADIKAL COZUM DEGISKENLERI ---
volatile int encCounter = 0; 
int lastProcessedCounter = 0;
unsigned long lastScrollTime = 0;
const unsigned long SETTLE_DELAY = 400; // 400ms bekleme
bool pendingStationChange = false;

int lastCLK = HIGH;
unsigned long swTime = 0;
unsigned long lastAct = 0;
unsigned long lastClk = 0;
unsigned long lastVU = 0;
unsigned long lastEncTime = 0;
unsigned long lastWeatherUpdate = 0;
int retryCount = 0;

// Interrupt Servis Routine (ISR) - IRAM'da calisir, cok hizlidir
void IRAM_ATTR encoderISR() {
  int clk = digitalRead(ENC_CLK);
  int dt = digitalRead(ENC_DT);
  if (clk == LOW) {
    if (dt == HIGH) encCounter++; else encCounter--;
  }
}


String weatherString = "Bekleniyor...";
String timeStr = "--:--";
String dateStr = "--/--/----";

int bytesRx = 0;
int vuLevel = 0;

#define MAX_RETRY 3

void setBL(int v) { ledcWrite(TFT_BL, v); }

// UTF-8'den ASCII'ye donusturucu (Ekrana turkce karakter basilabilmesi icin basit filter)
String filterTurkce(String text) {
  text.replace("ı", "i"); text.replace("İ", "I");
  text.replace("ç", "c"); text.replace("Ç", "C");
  text.replace("ş", "s"); text.replace("Ş", "S");
  text.replace("ö", "o"); text.replace("Ö", "O");
  text.replace("ü", "u"); text.replace("Ü", "U");
  text.replace("ğ", "g"); text.replace("Ğ", "G");
  return text;
}

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Guncel Open-Meteo API URL'si (Sicaklik ve Nem Icin)
    http.begin("http://api.open-meteo.com/v1/forecast?latitude=41.428&longitude=32.023&current=temperature_2m,relative_humidity_2m");
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      float temp = doc["current"]["temperature_2m"];
      int hum = doc["current"]["relative_humidity_2m"];
      
      // Caycuma, Derece ve Nem orani (Kompakt)
      weatherString = "CAYCUMA " + String(temp, 1) + "C %" + String(hum);
    }
    http.end();
  }
}

void updateTimeStrings() {
  struct tm t;
  if (getLocalTime(&t)) {
    char bTime[10]; sprintf(bTime, "%02d:%02d", t.tm_hour, t.tm_min);
    timeStr = String(bTime);
    char bDate[12]; sprintf(bDate, "%02d.%02d.%02d", t.tm_mday, t.tm_mon + 1, (t.tm_year + 1900) % 100);
    dateStr = String(bDate);
  }
}

// Ortalanmis yazi yazdiran yardimci fonksiyon
void printCentered(String text, int y, int size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  int w = text.length() * 6 * size; 
  int x = (160 - w) / 2;
  if(x < 0) x = 0;
  tft.setCursor(x, y);
  tft.print(text);
}

// ------ MODERN INTRO EKRANI ------
void drawBoot() {
  tft.fillScreen(C_BG);
  
  // Dikdortgen seklinde modern retro hoparlor stili
  tft.fillRoundRect(60, 10, 40, 24, 4, C_DARK);
  tft.drawRoundRect(60, 10, 40, 24, 4, C_BLUE);
  tft.fillCircle(73, 22, 7, C_BG); 
  tft.drawCircle(88, 26, 3, C_BG);
  
  // Ses dalgalari efekti
  tft.drawCircle(110, 20, 10, C_BLUE);
  tft.drawCircle(115, 17, 15, C_ACCENT);

  // 3 Satir Devasa Yazi (Ortali)
  printCentered("TA2CAY", 45, 2, C_TEXT);
  printCentered("INTERNET", 65, 2, C_ACCENT);
  printCentered("RADYOSU", 85, 2, C_TEXT);
}

void drawBootMsg(const char *msg, uint16_t c = C_GRAY) {
  tft.fillRect(0, 115, 160, 13, C_BG); // Mesaj yeri
  printCentered(msg, 115, 1, c);
}

void drawProgressBar(int progress) {
  int w = 120;
  int h = 4;
  int x = 20;
  int y = 108; // Bar yazinin altinda, duruma yakin
  tft.drawRect(x, y, w, h, C_BLUE);
  tft.fillRect(x, y, map(progress, 0, 100, 0, w), h, C_ACCENT);
}

// ------ MODERN ANA ARAYUZ ------

void drawHeader() {
  tft.fillRect(0, 0, 160, 26, C_HEADER); // Yükseklik 26'ya cikarildi (2 satir icin)
  tft.drawFastHLine(0, 25, 160, C_BLUE);

  tft.setTextSize(1);
  tft.setTextColor(C_TEXT);
  
  // 1. Satir Sol: Caycuma Hava Durumu
  tft.setCursor(2, 3);
  tft.print(weatherString); 

  // 1. Satir Sag: WiFi Gucu (Basit cubuklar)
  int rssi = WiFi.RSSI();
  uint16_t sigColor = (rssi > -65) ? C_ACCENT : ((rssi > -80) ? C_WARN : C_ERROR);
  tft.fillRect(145, 8, 3, 3, sigColor);  // 1. dis
  tft.fillRect(150, 5, 3, 6, sigColor);  // 2. dis
  if(rssi > -75) tft.fillRect(155, 2, 3, 9, sigColor); // 3. dis

  // 2. Satir Sol: Tarih ve Saat
  tft.setTextColor(C_GRAY);
  tft.setCursor(2, 14);
  tft.print(dateStr + " " + timeStr);
}

void drawFooter() {
  tft.fillRect(0, 110, 160, 18, C_FOOTER);
  tft.drawFastHLine(0, 109, 160, C_BLUE);

  tft.setTextSize(1);
  tft.setTextColor(C_GRAY);
  
  // Sol: "wi-fi: SSID"
  tft.setCursor(4, 114);
  String wName = (connectedWifi >= 0) ? String(wifiSSID[connectedWifi]) : "OFFLINE";
  if (wName.length() > 10) wName = wName.substring(0, 10);
  tft.print("wi-fi: " + wName);

  // Sag: "[Kategori]"
  String catName = filterTurkce(String(sGenre[curStation]));
  int w = catName.length() * 6 + 12; // Koseli parantezler dahil yaklasik genislik
  tft.setCursor(156 - w, 114); // Saga dayali
  tft.setTextColor(C_ACCENT);
  tft.print("[" + catName + "]");
}

void drawStationInfo() {
  // Arkaplani temizle (Header ve Footer arasinda kalan bolum, yukseklik azaldi)
  tft.fillRect(0, 26, 160, 83, C_BG); 
  
  // Radyo Ismi (Ortali, Tasmayan, Premium Font Gorunumu)
  String sNameStr = String(sName[curStation]);
  if (sNameStr.length() > 8) {
    // Isim cok uzunsa daha ufak yaz ki tasmadan okunsun
    printCentered(sNameStr, 40, 2, C_TEXT);
  } else {
    // Isim kisaysa buyuk yaz
    printCentered(sNameStr, 38, 3, C_TEXT);
  }

  // Ortaboy estetik eklentiler - Kanal ortalandi
  printCentered("KANAL: " + String(curStation + 1) + " / " + String(NSTATIONS), 65, 1, C_ACCENT);
  
  // Footer hemen altina basiliyor ki guncel kalsin
  drawFooter();
}

void drawStatusBox(String msg, uint16_t color) {
  // Baglaniyor/Sira gibi yazilarin uste binmesini onlemek icin temiz ve sabit kutu
  tft.fillRect(20, 78, 120, 16, C_BG); 
  tft.fillRoundRect(30, 78, 100, 14, 4, C_DARK);
  
  tft.setTextSize(1);
  tft.setTextColor(color);
  int w = msg.length() * 6;
  int x = 30 + (100 - w) / 2;
  tft.setCursor(x, 81);
  tft.print(msg);
}

void updateStatus(bool p) {
  isPlaying = p;
  if(p) {
    drawStatusBox("CALIYOR", C_ACCENT);
  } else {
    drawStatusBox("BAGLANIYOR...", C_WARN); 
  }
}

void drawVuMeter() {
  // Ortadaki VU metreyi yeni bir yatay spectrum seklinde yapiyoruz. (Orta-Alt yerlesim: Y:93)
  int yBase = 100;
  int xStart = 30; // 30 ile 130 arasinda ortada yatay bar
  int barWidth = 4;
  int barGap = 2;
  int numBars = 16;
  
  int tgt = 0;
  if (isPlaying && bytesRx > 0)
    tgt = constrain(map(bytesRx, 0, 300, 2, 10), 2, 10); // 10 birim yuksek
  bytesRx = 0;
  
  if (isPlaying) tgt = constrain(tgt + random(-3, 4), 1, 10);
  else tgt = 1; // Sadece noktalar
  
  vuLevel = (vuLevel + tgt) / 2; // Smooth out

  tft.fillRect(xStart, yBase - 10, 100, 12, C_BG); // Bar bolgesini temizle

  for (int i = 0; i < numBars; i++) {
    // Kenarlara dogru alcalan parabolik bir dalga efekti
    float env = 1.0 - abs(i - (numBars/2.0)) / (numBars/2.0); 
    int h = max(1, (int)(vuLevel * env + random(-1, 2)));
    
    if(!isPlaying) h = 1; // Calmiyorsa dumduz
    
    int x = xStart + i * (barWidth + barGap);
    uint16_t color = C_BLUE;
    if(h > 6) color = C_ACCENT;
    if(h > 8) color = C_WARN;
    
    tft.fillRect(x, yBase - h, barWidth, h, color);
  }
}

void drawMainUI() {
  tft.fillScreen(C_BG);
  drawHeader();
  drawStationInfo();
}

void skipHeaders() {
  while (activeClient->connected()) {
    String line = activeClient->readStringUntil('\n');
    if (line == "\r") break;
  }
}

void connectToStation(int idx) {
  httpClient.stop();
  httpsClient.stop();
  isPlaying = false;
  pendingStationChange = false; // Islem basladi
  
  drawMainUI();
  updateStatus(false);
  yield();

  bool connected = false;
  for(int attempt = 0; attempt < MAX_RETRY && !connected; attempt++) {
    yield();
    if(attempt > 0) {
      drawStatusBox("YENIDEN: " + String(attempt + 1), C_WARN);
      delay(500);
    }
    
    if (sUseTLS[idx]) {
      httpsClient.setInsecure();
      httpsClient.setTimeout(3000); // 3 saniye sert zaman asimi
      if (httpsClient.connect(hosts[idx], sPorts[idx])) {
        activeClient = &httpsClient;
        connected = true;
      }
    } else {
      httpClient.setTimeout(3000);
      if (httpClient.connect(hosts[idx], sPorts[idx])) {
        activeClient = &httpClient;
        connected = true;
      }
    }
  }
  
  if(!connected) {
    drawStatusBox("BAGLANTI HATASI", C_ERROR);
    return;
  }

  activeClient->print(String("GET ") + paths[idx] + " HTTP/1.0\r\n");
  activeClient->print(String("Host: ") + hosts[idx] + "\r\n");
  activeClient->print("Icy-MetaData: 0\r\n");
  activeClient->print("Connection: close\r\n\r\n");
  skipHeaders();
  updateStatus(true);
}

void setup() {
  Serial.begin(115200);
  
  // 1. GUC / PIN YAPILANDIRMASI (SPI oncesi guvenlik)
  pinMode(VS1053_XCS, OUTPUT);
  digitalWrite(VS1053_XCS, HIGH); // VS1053'u uykuda tut
  pinMode(VS1053_XDCS, OUTPUT);
  digitalWrite(VS1053_XDCS, HIGH);
  
  ledcAttach(TFT_BL, 5000, 8);
  setBL(255);

  // 2. SPI BASLAT
  SPI.begin();
  
  // 3. ONCE TFT BASLAT (Ve cizimi bitir)
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); 
  drawBoot();

  // 4. SONRA VS1053 BASLAT (Gecikmeli)
  delay(100);
  player.begin();
  player.switchToMp3Mode();
  player.setVolume(curVolume);

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  
  // v5.0 Interrupt Baglantisi
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, FALLING);
  
  connectedWifi = -1;
  WiFi.mode(WIFI_STA);
  
  for (int attempt = 0; attempt < WIFI_COUNT && connectedWifi < 0; attempt++) {
    WiFi.disconnect(true);
    delay(100);
    
    String msg = "WLAN: " + String(wifiSSID[attempt]);
    drawBootMsg(msg.c_str(), C_BLUE);
    
    WiFi.begin(wifiSSID[attempt], wifiPASS[attempt]);
    
    int wait = 0;
    while (WiFi.status() != WL_CONNECTED && wait < 15) {
      delay(500);
      wait++;
      drawProgressBar(map(wait, 0, 15, 0, 100));
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      connectedWifi = attempt;
      drawProgressBar(100);
      drawBootMsg("BAGLANTI BASARILI!", C_ACCENT);
      delay(500);
      break;
    } else {
      drawBootMsg("SONRAKI AG DENENIYOR...", C_WARN);
    }
  }
  
  if (connectedWifi < 0) {
    drawBootMsg("WIFI YOK! OFFLINE", C_ERROR);
    delay(2000);
  } else {
    drawBootMsg("SAAT ESIZLENIYOR...", C_TEXT);
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    delay(1500); // NTP'nin inmesi icin zaman ver
    drawBootMsg("HAVA DURUMU ALINIYOR...", C_TEXT);
    getWeather();
    updateTimeStrings();
  }
  
  drawBootMsg("SISTEM HAZIR!", C_ACCENT);
  delay(800);

  lastAct = millis();
  if (connectedWifi >= 0) {
    connectToStation(curStation);
  } else {
    drawMainUI();
  }
}

void checkInputs() {
  // 1. Encoder Hareket Kontrolü (Kesme ile sayilan deger)
  if (encCounter != lastProcessedCounter) {
    int diff = encCounter - lastProcessedCounter;
    lastProcessedCounter = encCounter;
    lastScrollTime = millis();
    pendingStationChange = true;
    lastAct = millis();
    
    // İstasyonu degistir (Henuz baglanmiyoruz, sadece UI guncellenecek)
    if (diff > 0) curStation = (curStation + 1) % NSTATIONS;
    else curStation = (curStation - 1 + NSTATIONS) % NSTATIONS;
    
    // ANLIK UI GÖSTERİMİ
    drawStationInfo(); 
    drawStatusBox("SECILIYOR...", C_BLUE);
  }

  // 2. Buton Kontrolü (Hala standart okuma yeterli)
  bool sw = digitalRead(ENC_SW);
  if (sw == LOW && millis() - swTime > 200) {
    swTime = millis();
    lastAct = millis();
    // Buton su an pasif veya farklı bir islem icin ayrılabilir
  }
}

void loop() {
  // v5.0 Settle-Delay: Çevirme bitti mi? 400ms gecti mi?
  if (pendingStationChange && (millis() - lastScrollTime > SETTLE_DELAY)) {
    connectToStation(curStation);
  }

  uint8_t buf[1024];
  while (activeClient->available()) {
    int b = activeClient->read(buf, sizeof(buf));
    if (b > 0) {
      player.playChunk(buf, b);
      bytesRx += b;
    }
    checkInputs();
    if (pendingStationChange) break; // Cevirme basladigi an calmayi kes ve UI'ya odaklan
  }

  if (activeClient->connected() == false && isPlaying) {
    updateStatus(false);
    connectToStation(curStation);
  }
  retryCount = 0;

  if (millis() - lastClk > 1000) {
    lastClk = millis();
    updateTimeStrings();
    drawHeader(); // Saat header'da oldugu icin header'i guncelliyoruz
    
    // Her 1.5 Saatte bir hava durumunu yenile
    if (millis() - lastWeatherUpdate > 5400000) { 
       lastWeatherUpdate = millis();
       getWeather();
       drawHeader(); // Header'daki havayi guncelle
    }
  }

  // Vu metre daha sik guncellensin (daha akici dalga efekti)
  if (millis() - lastVU > 150) {
    lastVU = millis();
    drawVuMeter();
  }

  checkInputs();
}
