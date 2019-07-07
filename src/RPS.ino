
#include <LoRaLib.h>
#include <SSD1306Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPmDNS.h>

#include "index_html.h"
#include "script_js.h"

#define USE_QUEUE 

#define OLED_ADDRESS 0x3c
#define OLED_SDA 4  // GPIO4
#define OLED_SCL 15 // GPIO15
#define OLED_RST 16 // GPIO16

static SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

#define LoRa_RST 14  // GPIO 14
#define LoRa_CS 18   // GPIO 18
#define LoRa_DIO0 26 // GPIO 26
#define LoRa_DIO1 33 // GPIO 33
#define LoRa_DIO2 32 // GPIO 32
SX1278 fsk = new LoRa(LoRa_CS, LoRa_DIO0, LoRa_DIO1);

#define WIFI_ACCESSPOINT false
#define WIFI_STATION true

#define ver_num 0x2
typedef struct {
  byte version;
  char wifi_ssid[33];
  char wifi_secret[65];
  char wifi_hostname[256];
  bool wifi_opmode;
  bool wifi_powersave;
  float tx_frequency;
  float tx_deviation;
  int8_t tx_power;
  uint8_t tx_current_limit;

  byte restaurant_id;
  byte system_id;
  byte alert_type;
} settings_t;

#define EEPROM_SIZE 4096

settings_t cfg;

typedef struct {
  int restaurant_id; 
  int system_id; 
  int pager_number; 
  int alert_type;
} pager_t;

unsigned long displayTime = millis();
unsigned long lastReconnect = -10000;
unsigned long buttonTime = -10000;
bool button_last_state = true;
SemaphoreHandle_t xSemaphore;
#ifdef USE_QUEUE
QueueHandle_t queue;
#endif
WebServer server(80);

void write_config(void)
{
  EEPROM.writeBytes(0, &cfg, sizeof(cfg));
  EEPROM.commit();
}

void read_config(void)
{
  EEPROM.readBytes(0, &cfg, sizeof(cfg));
  if (cfg.version != ver_num) {
    if (cfg.version == 0xff) {
      strcpy(cfg.wifi_ssid, "RPS");
      strcpy(cfg.wifi_secret, "");
      strcpy(cfg.wifi_hostname, "RPS");
      cfg.wifi_opmode = WIFI_ACCESSPOINT;
      cfg.wifi_powersave = false;
    }
    cfg.version = ver_num;
    cfg.restaurant_id = 0x0;
    cfg.system_id = 0x0;
    cfg.alert_type = 0x1;
    cfg.tx_current_limit = 100;
    cfg.tx_power = 17;
    cfg.tx_deviation = 3.5;
    cfg.tx_frequency = 446.146973;

    write_config();
  }
  printf("Settings:\n");
  printf("version         : %d\n", cfg.version);
  printf("ssid            : %s\n", cfg.wifi_ssid);
  printf("wifi_secret     : %s\n", cfg.wifi_secret);
  printf("wifi_hostname   : %s\n", cfg.wifi_hostname);
  printf("wifi_opmode     : %d\n", cfg.wifi_opmode);
  printf("wifi_powersave  : %d\n", cfg.wifi_powersave);
  printf("restaurant_id   : %d\n", cfg.restaurant_id);
  printf("system_id       : %d\n", cfg.system_id);
  printf("alert_type      : %d\n", cfg.alert_type);
  printf("tx_frequency    : %.6fMhz\n", cfg.tx_frequency);
  printf("tx_deviation    : %.1fkHz\n", cfg.tx_deviation);
  printf("tx_power        : %ddBm\n", cfg.tx_power);
  printf("tx_current_limit: %dmA\n", cfg.tx_current_limit);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

size_t generate_paging_code(byte *telegram, size_t telegram_len, byte restaurant_id, byte system_id, int pager_number, byte alert_type)
{
  if (telegram_len < 15) {
    return -1;
  }

  memset(telegram, 0, telegram_len);

  telegram[0] = 0xAA;
  telegram[1] = 0xAA;
  telegram[2] = 0xAA;
  telegram[3] = 0xFC;
  telegram[4] = 0x2D;
  telegram[5] = restaurant_id;
  telegram[6] = ((system_id << 4) & 0xf0) | ((pager_number >> 8) & 0xf);
  telegram[7] = pager_number;
  telegram[13] = alert_type;
  int crc = 0;
  for (int i = 0; i < 14; i++) {
    crc += telegram[i];
  }
  crc %= 255;
  telegram[14] = crc;

  printf("restaurant_id: %02x\n", restaurant_id);
  printf("system_id    : %02x\n", system_id);
  printf("pager_number : %02x\n", pager_number);
  printf("alert_type   : %02x\n", alert_type);
  printf("crc          : %02x\n", crc);

  return 15;
}

void call_pager(int restaurant_id, int system_id, int pager_number, int alert_type)
{
  byte txbuf[64];
  xSemaphoreTake(xSemaphore, portMAX_DELAY);

  displayTime = millis();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 0, "Paging");
  display.drawString(64, 30, String(pager_number));
  display.display();

  size_t len = generate_paging_code(txbuf, sizeof(txbuf), restaurant_id, system_id, pager_number, alert_type);
  memcpy(txbuf + len, txbuf, len);
  memcpy(txbuf + len * 2, txbuf, len);

  int state = fsk.transmit(txbuf, len * 3);
  if (state == ERR_NONE) {
    Serial.println(F("Packet transmitted successfully!"));
  }
  else if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F("Packet too long!"));
  }
  else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F("Timed out while transmitting!"));
  } else {
    Serial.println(F("Failed to transmit packet, code "));
    Serial.println(state);
  }

  xSemaphoreGive(xSemaphore);
}

#ifdef USE_QUEUE
void TaskCallPager( void *pvParameters){
  pager_t p;
  for(;;) {
    xQueueReceive(queue, &p, portMAX_DELAY);
    call_pager(p.restaurant_id, p.system_id, p.pager_number, p.alert_type);
    yield();
  }
}
#endif

void page(void)
{
  int pager_number = -1;
  byte alert_type = cfg.alert_type;
  byte restaurant_id = cfg.restaurant_id;
  byte system_id = cfg.system_id;
  String val = server.arg("pager_number");
  bool force = false;
  if (val) {
    pager_number = val.toInt();
  }
  if (server.hasArg("alert_type")) {
    alert_type = server.arg("alert_type").toInt();
  }
  if (server.hasArg("restaurant_id")) {
    restaurant_id = server.arg("restaurant_id").toInt();
  }
  if (server.hasArg("system_id")) {
    system_id = server.arg("system_id").toInt();
  }
  if (server.hasArg("force")) {
    force = server.arg("force").toInt();
  }

  if (pager_number > 0 || force) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "restaurant_id: " + String(restaurant_id) + " system_id: " + String(system_id) + " pager_number: " + String(pager_number) + " alert_type: " + String(alert_type));
    
    pager_t p;
    p.alert_type = alert_type;
    p.pager_number = pager_number;
    p.restaurant_id = restaurant_id;
    p.system_id = system_id;
#ifdef USE_QUEUE
    xQueueSend(queue, &p, portMAX_DELAY);
#else
    call_pager(p.restaurant_id, p.system_id, p.pager_number, p.alert_type);
#endif
  } else {
    server.send(200, "text/plain", "Invalid parameters supplied");
  }
}

void send_settings(void)
{
  DynamicJsonDocument json(1024);
  json["version"] = cfg.version;
  json["alert_type"] = cfg.alert_type;
  json["wifi_hostname"] = cfg.wifi_hostname;
  json["restaurant_id"] = cfg.restaurant_id;
  json["system_id"] = cfg.system_id;
  json["wifi_ssid"] = cfg.wifi_ssid;
  json["wifi_opmode"] = cfg.wifi_opmode;
  json["wifi_powersave"] = cfg.wifi_powersave;
  json["wifi_secret"] = cfg.wifi_secret;
  json["tx_frequency"] = String(cfg.tx_frequency,5);
  json["tx_deviation"] = cfg.tx_deviation;
  json["tx_power"] = cfg.tx_power;
  json["tx_current_limit"] = cfg.tx_current_limit;
  String output;
  serializeJson(json, output);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", output);
}

void parse_settings(void)
{
  DynamicJsonDocument json(1024);
  String str = server.arg("plain");
  printf("body: %s", str.c_str());
  DeserializationError error = deserializeJson(json, server.arg("plain"));
  if (error) {
    server.send(200, "text/plain", "deserializeJson failed");
  }
  else {
    if (json.containsKey("alert_type"))
      cfg.alert_type = json["alert_type"];
    if (json.containsKey("wifi_hostname"))
      strcpy(cfg.wifi_hostname, json["wifi_hostname"]);
    if (json.containsKey("restaurant_id"))
      cfg.restaurant_id = json["restaurant_id"];
    if (json.containsKey("system_id"))
      cfg.system_id = json["system_id"];
    if (json.containsKey("wifi_ssid"))
      strcpy(cfg.wifi_ssid, json["wifi_ssid"]);
    if (json.containsKey("wifi_opmode"))
      cfg.wifi_opmode = json["wifi_opmode"];
    if (json.containsKey("wifi_powersave"))
      cfg.wifi_powersave = json["wifi_powersave"];
    if (json.containsKey("wifi_secret"))
      strcpy(cfg.wifi_secret, json["wifi_secret"]);
    if (json.containsKey("tx_frequency"))
      cfg.tx_frequency = json["tx_frequency"];
    if (json.containsKey("tx_deviation"))
      cfg.tx_deviation = json["tx_deviation"];
    if (json.containsKey("tx_power"))
      cfg.tx_power = json["tx_power"];
    if (json.containsKey("tx_current_limit"))
      cfg.tx_current_limit = json["tx_current_limit"];

    write_config();
    send_settings();
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.print(F("Initializing ... "));

  EEPROM.begin(EEPROM_SIZE);
  read_config();

  xSemaphore = xSemaphoreCreateBinary();
  if ((xSemaphore) != NULL) {
    xSemaphoreGive(xSemaphore);
  }
#ifdef USE_QUEUE
  queue = xQueueCreate( 10, sizeof( pager_t )*2 );
  if(queue == NULL){
    Serial.println("Error creating the queue");
  }
  xTaskCreate(TaskCallPager, "TaskCallPager", 2048, NULL, 10, NULL);
#endif
  pinMode(GPIO_NUM_0, INPUT_PULLUP);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); // low to reset OLED
  delay(50);
  digitalWrite(OLED_RST, HIGH); // must be high to turn on OLED

  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(64, 8, "Version: 1.0");
  display.drawString(64,16, "WIFI: " + String((cfg.wifi_opmode==WIFI_STATION)?"STA":"AP"));
  display.drawString(64,24, "SSID: " + String(cfg.wifi_ssid));
  display.drawString(64,32, "HOSTNAME: " + String(cfg.wifi_hostname));
  display.display();

  if (cfg.wifi_opmode == WIFI_STATION) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);
    WiFi.setSleep(cfg.wifi_powersave);
    WiFi.setHostname(cfg.wifi_hostname);
    Serial.println("");

    unsigned long lastConnect = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if ((millis() - lastConnect) > 10000) {
        cfg.wifi_opmode = WIFI_ACCESSPOINT;
        break;
      }
    }
    if (cfg.wifi_opmode == WIFI_STATION) {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(cfg.wifi_ssid);
      Serial.print("STA IP address: ");
      Serial.println(WiFi.localIP());

      displayTime = millis();
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 40, "IP: " + WiFi.localIP().toString());
      display.display();
    } else {
      WiFi.disconnect();
      printf("Failed to connect to SSID %s falling back to AP mode\n", cfg.wifi_ssid);
    }
  }
  if (cfg.wifi_opmode == WIFI_ACCESSPOINT) {
    WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 40, "IP: " + IP.toString());
    display.display();
  }
  if (MDNS.begin(cfg.wifi_hostname)) {
    Serial.println("MDNS responder started");
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "text/html", ___data_index_html, ___data_index_html_len);
  });
  server.on("/script.js", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "application/javascript", ___data_script_js, ___data_script_js_len);
  });

  server.on("/page", page);
  server.on("/settings.json", HTTP_GET, send_settings);
  server.on("/settings.json", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-control-Allow-Credentials", "false");
    server.sendHeader("Access-control-Allow-Headers", "x-requested-with");
    server.sendHeader("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type");

    server.send(204);
  });
  server.on("/settings.json", HTTP_POST, parse_settings);
  server.on("/reboot", []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "OK");
    EEPROM.commit();
    sleep(1);
    ESP.restart();
  });
  server.on("/factoryreset", []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "OK");
    cfg.version = 0xff;
    write_config();
    sleep(1);
    ESP.restart();
  });
  server.onNotFound(handleNotFound);

  server.begin();

  int state = fsk.beginFSK(cfg.tx_frequency, 0.622, cfg.tx_deviation, 250, cfg.tx_power, cfg.tx_current_limit, 0, false);
  if (state == ERR_NONE) {
    Serial.println(F("beginFSK success!"));
  } else {
    Serial.print(F("beginFSK failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop()
{
  server.handleClient();

  if (cfg.wifi_opmode == WIFI_STATION && WiFi.status() == 6) {
    if (millis() - lastReconnect > 5000) {
      printf("+++++++++++++ trying to reconnect +++++++++++++");
      WiFi.reconnect();
      WiFi.setHostname(cfg.wifi_hostname);
      WiFi.setSleep(cfg.wifi_powersave);
      lastReconnect = millis();
    }
  }

  if (millis() - displayTime > 5000) {
    display.clear();
    display.display();
  }

  if (!digitalRead(GPIO_NUM_0) && button_last_state) {
    buttonTime = millis();
    button_last_state = false;
    printf("Button pressed\n");
  }
  if (digitalRead(GPIO_NUM_0) && !button_last_state) {
    buttonTime = millis();
    button_last_state = true;
    printf("Button released\n");
  }
  if (!digitalRead(GPIO_NUM_0) && !button_last_state && (millis() - buttonTime) > 3000) {
    printf("RESET Config\n");
    cfg.version = 0xff;
    write_config();
    button_last_state = true;
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 32, "FACTORY RESET");
    display.display();

    sleep(1);
    ESP.restart();
  }
}
