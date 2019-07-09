#include "rps.h"

static SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
SX1278 fsk = new LoRa(LoRa_CS, LoRa_DIO0, LoRa_DIO1);
settings_t cfg;

static unsigned long displayTime = millis();
static unsigned long displayCleared = millis();
static unsigned long lastReconnect = -10000;
static unsigned long buttonTime = -10000;
static bool button_last_state = true;
static SemaphoreHandle_t xSemaphore;
#ifdef USE_QUEUE
static QueueHandle_t queue;
#endif
static WebServer server(80);


static void handleNotFound()
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

static void display_updated(void)
{
  displayTime = millis();
  displayCleared = 0;
}

static int call_pager(byte mode, int restaurant_id, int system_id, int pager_number, int alert_type, bool reprogram_pager)
{
  int ret = -1;
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  
  display_updated();

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  if(!reprogram_pager) {
    display.drawString(64, 12, "Paging");
  } else {
    display.drawString(64, 12, "Reprog");
  } 
  display.drawString(64, 42, String(pager_number));
  display.display();
  switch(mode) {
    case 0:
      ret = lrs_pager(fsk, restaurant_id, system_id, pager_number, alert_type, reprogram_pager);
      break;
    case 1:
      ret = pocsag_pager(fsk, cfg.pocsag_baud, pager_number, alert_type, FUNC_BEEP, NULL);
      break;
    default: break;
  }
  xSemaphoreGive(xSemaphore);
  return ret;
}

#ifdef USE_QUEUE
void TaskCallPager( void *pvParameters){
  pager_t p;
  for(;;) {
    xQueueReceive(queue, &p, portMAX_DELAY);
    call_pager(p.restaurant_id, p.system_id, p.pager_number, p.alert_type, false);
    yield();
  }
}
#endif

static void page(void)
{
  int pager_number = -1;
  byte alert_type = cfg.alert_type;
  byte restaurant_id = cfg.restaurant_id;
  byte system_id = cfg.system_id;
  String val = server.arg("pager_number");
  bool force = false;
  bool reprog = false;
  byte mode = cfg.default_mode;

  if (val) {
    pager_number = val.toInt();
    pager_number = abs(pager_number)&0xfff;
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
  if (server.hasArg("reprogram")) {
    reprog=server.arg("reprogram").toInt();
  }
  if (server.hasArg("mode")) {
    mode = server.arg("mode").toInt();
  }

  if (pager_number > 0 || force) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "restaurant_id: " + String(restaurant_id) + " system_id: " + String(system_id) + " pager_number: " + String(pager_number) + " alert_type: " + String(alert_type));
    
    if(!reprog) {
#ifdef USE_QUEUE
      pager_t p;
      p.alert_type = alert_type;
      p.pager_number = pager_number;
      p.restaurant_id = restaurant_id;
      p.system_id = system_id;
      xQueueSend(queue, &p, portMAX_DELAY);
#else
      call_pager(mode, restaurant_id, system_id, pager_number, alert_type, false);
#endif
    } else {
      call_pager(mode, restaurant_id, system_id, pager_number, alert_type, true);
    }
  } else {
    server.send(200, "text/plain", "Invalid parameters supplied");
  }
}

static void send_settings(void)
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
  json["default_mode"] = cfg.default_mode;
  json["pocsag_baud"] = cfg.pocsag_baud;

  String output;
  serializeJson(json, output);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", output);
}

static void parse_settings(void)
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
    if (json.containsKey("pocsag_baud"))
      cfg.pocsag_baud = json["pocsag_baud"];
    if (json.containsKey("default_mode"))
      cfg.default_mode = json["default_mode"];

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
  queue = xQueueCreate( 10, sizeof( pager_t ) );
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
  display.drawString(64, 8, "Restaurant Paging Service");
  display.drawString(64,16, "Version: "+ String(VERSION_STR));
  display.drawString(64,24, "WIFI: " + String((cfg.wifi_opmode==WIFI_STATION)?"STA":"AP"));
  display.drawString(64,32, "SSID: " + String(cfg.wifi_ssid));
  display.drawString(64,40, "HOSTNAME: " + String(cfg.wifi_hostname));
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

      display_updated();
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 48, "IP: " + WiFi.localIP().toString());
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
    display.drawString(64, 48, "IP: " + IP.toString());
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
  state |= fsk.setDCFree(SX127X_DC_FREE_MANCHESTER);
  state |= fsk.setCRC(false);
  if (state == ERR_NONE) {
    Serial.println(F("beginFSK success!"));
  } else {
    Serial.print(F("beginFSK failed, code "));
    Serial.println(state);
    while (true);
  }
  pocsag_setup();
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

  if ((millis() - displayTime > 5000) && !displayCleared) {
    displayCleared = millis();
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
