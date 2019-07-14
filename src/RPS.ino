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
static WebServer server(80);
static EOTAUpdate *updater;
#ifdef USE_QUEUE
static QueueHandle_t queue;
#endif


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

static int call_pager(byte mode, int tx_power, float tx_frequency, float tx_deviation, int pocsag_baud, int restaurant_id, int system_id, int pager_number, int alert_type, bool reprogram_pager, func_t pocsag_telegram_type, const char *message)
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
      ret = lrs_pager(fsk, tx_power, tx_frequency, tx_deviation, restaurant_id, system_id, pager_number, alert_type, reprogram_pager);
      break;
    case 1:
      ret = pocsag_pager(fsk, tx_power, tx_frequency, tx_deviation, pocsag_baud, pager_number, alert_type, pocsag_telegram_type, message);
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
  float tx_frequency = cfg.tx_frequency;
  byte alert_type = cfg.alert_type;
  byte restaurant_id = cfg.restaurant_id;
  byte system_id = cfg.system_id;
  bool force = false;
  bool reprog = false;
  byte mode = cfg.default_mode;
  int pocsag_baud = cfg.pocsag_baud;
  int tx_power = cfg.tx_power;
  float tx_deviation = cfg.tx_deviation;
  func_t pocsag_telegram_type = FUNC_BEEP;
  String message;

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
  if (server.hasArg("pager_number")) {
    pager_number = server.arg("pager_number").toInt();
    if(mode == 0) {
      pager_number = abs(pager_number)&0xfff;
    }
  }
  if (server.hasArg("tx_frequency")) {
    tx_frequency = server.arg("tx_frequency").toFloat();
  }
  if (server.hasArg("tx_deviation")) {
    tx_deviation = server.arg("tx_deviation").toFloat();
  }
  if (server.hasArg("tx_power")) {
    tx_power = server.arg("tx_power").toInt();
  }
  if (server.hasArg("pocsag_baud")) {
    pocsag_baud = server.arg("pocsag_baud").toInt();
  }
  if (server.hasArg("pocsag_telegram_type")) {
    pocsag_telegram_type = (func_t)server.arg("pocsag_telegram_type").toInt();
  }
  if (server.hasArg("message")) {
    message = server.arg("message");
  }
 
  if (pager_number > 0 || force) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String str="mode: "+String(mode)+"\ntx_power: "+String(tx_power)+"\ntx_frequency: "+String(tx_frequency,5)+
               "\ntx_deviation: "+String(tx_deviation)+ "\npocsag_baud: "+String(pocsag_baud)+ 
               "\nrestaurant_id: "+String(restaurant_id)+ "\nsystem_id: "+String(system_id)+ 
               "\npager_number: "+String(pager_number)+ "\nalert_type: "+String(alert_type)+ 
               "\npocsag_telegram_type: "+String(pocsag_telegram_type)+"\nmessage: "+message;

    server.send(200, "text/plain", str);
    
    if(!reprog) {
#ifdef USE_QUEUE
      pager_t p;
      p.alert_type = alert_type;
      p.pager_number = pager_number;
      p.restaurant_id = restaurant_id;
      p.system_id = system_id;
      xQueueSend(queue, &p, portMAX_DELAY);
#else
      call_pager(mode, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, false, pocsag_telegram_type, message.c_str());
#endif
    } else {
      call_pager(mode, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, true, pocsag_telegram_type, message.c_str());
    }
  } else {
    server.send(400, "text/plain", "Invalid parameters supplied");
  }
}

static void send_settings(void)
{
  DynamicJsonDocument json(1024);
  json["version"] = VERSION_STR;
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
  json["ota_path"] = cfg.ota_path;

  String output;
  serializeJson(json, output);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", output);
}

static void parse_settings(void)
{
  DynamicJsonDocument json(1024);
  String str = server.arg("plain");
#ifdef DEBUG
  printf("body: %s", str.c_str());
#endif
  DeserializationError error = deserializeJson(json, server.arg("plain"));
  if (error) {
    server.send(200, "text/plain", "deserializeJson failed");
  }
  else {
    if (json.containsKey("alert_type"))
      cfg.alert_type = json["alert_type"];
    if (json.containsKey("wifi_hostname"))
      strncpy(cfg.wifi_hostname, json["wifi_hostname"],sizeof(cfg.wifi_hostname));
    if (json.containsKey("restaurant_id"))
      cfg.restaurant_id = json["restaurant_id"];
    if (json.containsKey("system_id"))
      cfg.system_id = json["system_id"];
    if (json.containsKey("wifi_ssid"))
      strncpy(cfg.wifi_ssid, json["wifi_ssid"], sizeof(cfg.wifi_ssid));
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
    if (json.containsKey("ota_path"))
      strncpy(cfg.ota_path, json["ota_path"], sizeof(cfg.ota_path));

    write_config();
    send_settings();
  }
}

void setup()
{
  Serial.begin(115200);
  printf("Version: %s, Version Number: %d, CFG Number: %d\n",VERSION_STR, VERSION_NUMBER, cfg_ver_num);
  printf("Initializing ... ");

  EEPROM.begin(EEPROM_SIZE);
  read_config();
  updater= new EOTAUpdate(cfg.ota_path, VERSION_NUMBER);
  xSemaphore = xSemaphoreCreateBinary();
  if ((xSemaphore) != NULL) {
    xSemaphoreGive(xSemaphore);
  }
#ifdef USE_QUEUE
  queue = xQueueCreate( 10, sizeof( pager_t ) );
  if(queue == NULL){
    printf("Error creating the queue\n");
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
  display.drawString(64, 4, "Restaurant Paging Service");
  display.drawString(64,14, "Version: "+ String(VERSION_STR));
  display.drawString(64,24, "WIFI: " + String((cfg.wifi_opmode==WIFI_STATION)?"STA":"AP"));
  display.drawString(64,34, "SSID: " + String(cfg.wifi_ssid));
  display.drawString(64,44, "HOSTNAME: " + String(cfg.wifi_hostname));
  display.display();

  if (cfg.wifi_opmode == WIFI_STATION) {
    WiFi.setSleep(cfg.wifi_powersave);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(cfg.wifi_hostname);
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);
    printf("\n");

    unsigned long lastConnect = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      printf(".");
      if ((millis() - lastConnect) > 10000) {
        cfg.wifi_opmode = WIFI_ACCESSPOINT;
        break;
      }
    }
    if (cfg.wifi_opmode == WIFI_STATION) {
      printf("\n");
      printf("Connected to %s\n", cfg.wifi_ssid);
      printf("STA IP address: %s\n", WiFi.localIP().toString().c_str());

      display_updated();
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 54, "IP: " + WiFi.localIP().toString());
      display.display();
    } else {
      WiFi.disconnect();
      printf("\nFailed to connect to SSID %s falling back to AP mode\n", cfg.wifi_ssid);
      strcpy(cfg.wifi_ssid,"RPS");
      cfg.wifi_secret[0]=0;
    }
  }
  if (cfg.wifi_opmode == WIFI_ACCESSPOINT) {
    WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
    IPAddress IP = WiFi.softAPIP();
    printf("AP IP address: %s\n", IP.toString().c_str());
    display.setFont(ArialMT_Plain_10);
    for(int x = 0;x < 128; x++) {
      for(int y = 0; y < 10; y++) {
            display.clearPixel(x,y+24);
      }
    }
    display.drawString(64, 24, "WIFI: " + String((cfg.wifi_opmode==WIFI_STATION)?"STA":"AP"));
    display.drawString(64, 54, "IP: " + IP.toString());
    display.display();
  }
  if (!MDNS.begin(cfg.wifi_hostname)) {
    printf("MDNS responder failed to start\n");
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
  int state = fsk.beginFSK(cfg.tx_frequency, 0.622, cfg.tx_deviation, 10, cfg.tx_power, cfg.tx_current_limit, 0, false);
  state |= fsk.setDCFree(SX127X_DC_FREE_MANCHESTER);
  state |= fsk.setCRC(false);
  if (state != ERR_NONE) {
    printf("beginFSK failed, code %d\n", state);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 12, "SX127X");
    display.drawString(64, 42, "FAIL");
    display.display();
    while(true) {
      yield();
    }
  }
  pocsag_setup();
}

static void check_buttons()
{
  if (!digitalRead(GPIO_NUM_0) && button_last_state) {
    buttonTime = millis();
    button_last_state = false;
  }
  if (digitalRead(GPIO_NUM_0) && !button_last_state) {
    buttonTime = millis();
    button_last_state = true;
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

static void check_wifi()
{
    if (WiFi.getMode() == WIFI_MODE_STA && WiFi.status() == WL_DISCONNECTED) {
    if (millis() - lastReconnect > 5000) {
      printf("lost WIFI connecion - trying to reconnect\n");
      WiFi.reconnect();
      lastReconnect = millis();
    }
  }
}

static void check_display()
{
  if ((millis() - displayTime > 5000) && !displayCleared) {
    displayCleared = millis();
    display.clear();
    display.display();
  }
}

void loop()
{
  server.handleClient();

  check_display();
  check_wifi();
  check_buttons();

  if(cfg.ota_path[0] && WiFi.getMode() == WIFI_MODE_STA && WiFi.status() == WL_CONNECTED) {
    updater->CheckAndUpdate();
  }
}
