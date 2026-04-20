#include <SPI.h>
#include <LoRa.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ╔══════════════════════════════════════════════════════════╗
// ║  🔧 USER CONFIGURABLE PINS                               ║
// ╚══════════════════════════════════════════════════════════╝
const int PIN_MODE_SWITCH = 25;    // switch → GND (closed = ESP-NOW)
const int PIN_LED         = 4;     // LED → 220Ω → GND

// LoRa pins
const int PIN_LORA_SS   = 5;
const int PIN_LORA_RST  = 14;
const int PIN_LORA_DIO0 = 2;

// ╔══════════════════════════════════════════════════════════╗
// ║  📡 AIR UNIT MAC ADDRESS — এখানে বসাও!                   ║
// ╠══════════════════════════════════════════════════════════╣
// ║                                                          ║
// ║  STEPS:                                                  ║
// ║  1. MAC Finder sketch AIR ESP32 এ upload করো            ║
// ║  2. Serial Monitor (115200) এ MAC দেখো                  ║
// ║  3. সেই MAC নিচের array তে paste করো                    ║
// ║                                                          ║
// ║  উদাহরণ: যদি Air MAC হয় A1:B2:C3:D4:E5:F6              ║
// ║  তাহলে নিচে এভাবে লিখবে:                                ║
// ║  {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}                   ║
// ║                                                          ║
// ╚══════════════════════════════════════════════════════════╝
uint8_t airMac[] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
//                    👆 এই 6 টা value AIR এর MAC দিয়ে replace করো

// ============================================================
// SETTINGS
// ============================================================
#define PC_BAUD         57600        // Mission Planner connect baud

// LoRa RF (Air এর সাথে EXACT match হতে হবে)
#define LORA_FREQ       433E6
#define LORA_SF         7
#define LORA_BW         500E3
#define LORA_CR         5
#define LORA_TX_PW      17

// Framing
#define SYNC_BYTE       0xAA
#define MAX_PAYLOAD     58
#define FT_DATA         0x01
#define FT_KEEPALIVE    0x02
#define REPLY_GUARD_US  700

// ============================================================
// 💡 LED LOGIC
//    LoRa mode    → LED ON
//    ESP-NOW mode → LED OFF
// ============================================================
#define LED_ON_FOR_LORA   HIGH
#define LED_ON_FOR_ESPNOW LOW

// PC ring buffer
#define PC_BUF_SIZE     1024
uint8_t  pcBuf[PC_BUF_SIZE];
volatile int pcHead = 0, pcTail = 0;

static inline int  pcAvail() { return (pcHead - pcTail + PC_BUF_SIZE) % PC_BUF_SIZE; }
static inline void pcPush(uint8_t b) {
  int n = (pcHead + 1) % PC_BUF_SIZE;
  if (n != pcTail) { pcBuf[pcHead] = b; pcHead = n; }
}
static inline uint8_t pcPop() {
  uint8_t b = pcBuf[pcTail]; pcTail = (pcTail + 1) % PC_BUF_SIZE; return b;
}
static inline void pcClear() { pcHead = pcTail = 0; }
static inline void drainPC() { while (Serial.available()) pcPush(Serial.read()); }

uint8_t crc8(const uint8_t *d, int n) {
  uint8_t c = 0xFF;
  for (int i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1);
  }
  return c;
}

// State
enum Mode { MODE_LORA, MODE_ESPNOW };
volatile Mode currentMode = MODE_LORA;
bool          lastSwState = false;
unsigned long swPollTs    = 0;
#define SW_POLL_MS        30

uint8_t txFrame[MAX_PAYLOAD + 4];

// ESP-NOW RX flag
volatile int espRxLen = 0;
uint8_t      espRxBuf[250];

// ============================================================
// LoRa
// ============================================================
void loraStart() {
  SPI.begin(18, 19, 23, PIN_LORA_SS);
  LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);
  while (!LoRa.begin(LORA_FREQ)) delay(200);
  LoRa.setTxPower(LORA_TX_PW);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setPreambleLength(6);
  LoRa.setSyncWord(0x34);
  LoRa.enableCrc();
  LoRa.receive();
}
void loraStop() { LoRa.sleep(); LoRa.end(); }

void loraReply() {
  drainPC();
  int n = pcAvail(); if (n > MAX_PAYLOAD) n = MAX_PAYLOAD;

  txFrame[0] = SYNC_BYTE;
  txFrame[1] = (n > 0) ? FT_DATA : FT_KEEPALIVE;
  txFrame[2] = (uint8_t)n;
  for (int i = 0; i < n; i++) txFrame[3 + i] = pcPop();
  txFrame[3 + n] = crc8(txFrame, 3 + n);

  LoRa.beginPacket();
  LoRa.write(txFrame, 3 + n + 1);
  LoRa.endPacket(false);
  LoRa.receive();
}

bool loraRx() {
  int pkt = LoRa.parsePacket();
  if (pkt == 0) return false;
  if (pkt < 4)  { while (LoRa.available()) LoRa.read(); return false; }

  uint8_t hdr[3]; LoRa.readBytes(hdr, 3);
  if (hdr[0] != SYNC_BYTE) { while (LoRa.available()) LoRa.read(); return false; }

  uint8_t len = hdr[2];
  if (len > MAX_PAYLOAD || LoRa.available() < (int)len + 1) {
    while (LoRa.available()) LoRa.read(); return false;
  }
  static uint8_t pl[MAX_PAYLOAD];
  LoRa.readBytes(pl, len);
  uint8_t rxCrc = LoRa.read();

  uint8_t tmp[3 + MAX_PAYLOAD];
  memcpy(tmp, hdr, 3); memcpy(&tmp[3], pl, len);
  if (crc8(tmp, 3 + len) != rxCrc) return false;

  if (hdr[1] == FT_DATA && len > 0) Serial.write(pl, len);
  return true;
}

// ============================================================
// ESP-NOW
// ============================================================
void onEspRx(const esp_now_recv_info *info, const uint8_t *d, int len) {
  if (len > 0 && len <= 250) {
    memcpy(espRxBuf, d, len);
    espRxLen = len;
  }
}

void espStart() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(onEspRx);

  // 📡 Air unit কে peer হিসেবে add করছি
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, airMac, 6);       // ← এখানে airMac use হচ্ছে
  p.channel = 0; p.encrypt = false; p.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&p);
}

void espStop() {
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(30);
}

// ============================================================
// Mode switch
// ============================================================
void enterLora() {
  espStop();
  pcClear();
  loraStart();
  digitalWrite(PIN_LED, LED_ON_FOR_LORA);
  currentMode = MODE_LORA;
}
void enterEspnow() {
  loraStop();
  pcClear();
  espStart();
  digitalWrite(PIN_LED, LED_ON_FOR_ESPNOW);
  currentMode = MODE_ESPNOW;
}

// ============================================================
// Setup / Loop
// ============================================================
void setup() {
  pinMode(PIN_MODE_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  Serial.begin(PC_BAUD);
  Serial.setRxBufferSize(2048);

  lastSwState = (digitalRead(PIN_MODE_SWITCH) == LOW);
  if (lastSwState) enterEspnow(); else enterLora();
}

void loop() {
  unsigned long now = millis();

  // ---- switch poll ----
  if (now - swPollTs >= SW_POLL_MS) {
    swPollTs = now;
    bool s = (digitalRead(PIN_MODE_SWITCH) == LOW);
    if (s != lastSwState) {
      lastSwState = s;
      s ? enterEspnow() : enterLora();
      return;
    }
  }

  // ============= ESP-NOW MODE =============
  if (currentMode == MODE_ESPNOW) {
    // 1. flush received bytes to MP
    if (espRxLen > 0) {
      noInterrupts();
      int len = espRxLen;
      espRxLen = 0;
      interrupts();
      Serial.write(espRxBuf, len);
    }

    // 2. forward MP → air
    int avail = Serial.available();
    if (avail > 0) {
      static uint8_t buf[240];
      if (avail > 240) avail = 240;
      Serial.readBytes(buf, avail);
      esp_now_send(airMac, buf, avail);    // ← airMac এ পাঠাচ্ছে
    }
    return;
  }

  // ============= LoRa MODE =============
  drainPC();
  if (loraRx()) {
    delayMicroseconds(REPLY_GUARD_US);
    loraReply();
  }
}
