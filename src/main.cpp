#include "rps.h"

static SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
SX1278 fsk = new LoRa(LoRa_CS, LoRa_DIO0, LoRa_DIO1);
settings_t cfg;

static unsigned long displayTime = millis();
static unsigned long displayCleared = millis();
static unsigned long lastReconnect = -10000;
static unsigned long lastDisconnect = -10000;
static unsigned long buttonTime = -10000;
static bool button_last_state = true;
static SemaphoreHandle_t xSemaphore;
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static AsyncEventSource events("/events");
static EOTAUpdate *updater;
#ifdef USE_QUEUE
static QueueHandle_t queue;
#endif


static void handleNotFound(AsyncWebServerRequest *request)
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  for (uint8_t i = 0; i < request->args(); i++)
  {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  request->send(404, "text/plain", message);
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

static void page(AsyncWebServerRequest *request)
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
  
  if (request->hasArg("alert_type")) {
    alert_type = request->arg("alert_type").toInt();
  }
  if (request->hasArg("restaurant_id")) {
    restaurant_id = request->arg("restaurant_id").toInt();
  }
  if (request->hasArg("system_id")) {
    system_id = request->arg("system_id").toInt();
  }
  if (request->hasArg("force")) {
    force = request->arg("force").toInt();
  }
  if (request->hasArg("reprogram")) {
    reprog=request->arg("reprogram").toInt();
  }
  if (request->hasArg("mode")) {
    mode = request->arg("mode").toInt();
  }
  if (request->hasArg("pager_number")) {
    pager_number = request->arg("pager_number").toInt();
    if(mode == 0) {
      pager_number = abs(pager_number)&0xfff;
    }
  }
  if (request->hasArg("tx_frequency")) {
    tx_frequency = request->arg("tx_frequency").toFloat();
  }
  if (request->hasArg("tx_deviation")) {
    tx_deviation = request->arg("tx_deviation").toFloat();
  }
  if (request->hasArg("tx_power")) {
    tx_power = request->arg("tx_power").toInt();
  }
  if (request->hasArg("pocsag_baud")) {
    pocsag_baud = request->arg("pocsag_baud").toInt();
  }
  if (request->hasArg("pocsag_telegram_type")) {
    pocsag_telegram_type = (func_t)request->arg("pocsag_telegram_type").toInt();
  }
  if (request->hasArg("message")) {
    message = request->arg("message");
  }
 
  if (pager_number > 0 || force) {
    String str="mode: "+String(mode)+"\ntx_power: "+String(tx_power)+"\ntx_frequency: "+String(tx_frequency,5)+
               "\ntx_deviation: "+String(tx_deviation)+ "\npocsag_baud: "+String(pocsag_baud)+ 
               "\nrestaurant_id: "+String(restaurant_id)+ "\nsystem_id: "+String(system_id)+ 
               "\npager_number: "+String(pager_number)+ "\nalert_type: "+String(alert_type)+ 
               "\npocsag_telegram_type: "+String(pocsag_telegram_type)+"\nmessage: "+message;

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", str);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    
    if(!reprog) {
      call_pager(mode, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, false, pocsag_telegram_type, message.c_str());
    } else {
      call_pager(mode, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, true, pocsag_telegram_type, message.c_str());
    }
  } else {
    request->send(400, "text/plain", "Invalid parameters supplied");
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    //client connected
    info("ws[%s][%u] connect\n", server->url(), client->id());
    String str = get_settings();
    client->printf("%s", str.c_str());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    //client disconnected
    info("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    //error was received from the other end
    info("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    //pong message was received (in response to a ping request maybe)
    info("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    //data packet
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      //info("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
      if (info->opcode == WS_TEXT)
      {
        data[len] = 0;
        info("data: %s\n", (char *)data);
//        parse_cmd((char *)data, client);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  info("Version: %s, Version Number: %d, CFG Number: %d\n",VERSION_STR, VERSION_NUMBER, cfg_ver_num);
  info("Initializing ... ");

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
    info("Error creating the queue\n");
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
  display.drawString(64,44, "NAME: " + String(cfg.wifi_hostname));
  display.display();

  if (cfg.wifi_opmode == WIFI_STATION) {
    WiFi.disconnect();
    WiFi.setAutoReconnect(true);
    WiFi.setHostname(cfg.wifi_hostname);
    WiFi.setSleep(cfg.wifi_powersave);
    WiFi.mode(WIFI_STA);

    IPAddress myIP;
    IPAddress myGW;
    IPAddress myNM;
    IPAddress myDNS;

    myIP.fromString(cfg.ip_addr);
    myGW.fromString(cfg.ip_gw);
    myNM.fromString(cfg.ip_netmask);
    myDNS.fromString(cfg.ip_dns);

    WiFi.config(myIP, myGW, myNM, myDNS);
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);
    info("\n");

    unsigned long lastConnect = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      info(".");
      if (((millis() - lastConnect) > 10000) && cfg.wifi_ap_fallback) {
        cfg.wifi_opmode = WIFI_ACCESSPOINT;
        break;
      }
    }
    if (cfg.wifi_opmode == WIFI_STATION) {
      info("\n");
      info("Connected to %s\n", cfg.wifi_ssid);
      info("STA IP address: %s\n", WiFi.localIP().toString().c_str());

      display_updated();
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 54, "IP: " + WiFi.localIP().toString());
      display.display();
    } else {
      WiFi.disconnect();
      info("\nFailed to connect to SSID %s falling back to AP mode\n", cfg.wifi_ssid);
      strcpy(cfg.wifi_ssid,"RPS");
      cfg.wifi_secret[0]=0;
    }
  }
  if (cfg.wifi_opmode == WIFI_ACCESSPOINT) {
    WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
    IPAddress IP = WiFi.softAPIP();
    info("AP IP address: %s\n", IP.toString().c_str());
    display.setFont(ArialMT_Plain_10);
    for(int x = 0;x < 128; x++) {
      for(int y = 0; y < 20; y++) {
            display.clearPixel(x,y+24);
      }
    }
    display.drawString(64, 24, "WIFI: " + String((cfg.wifi_opmode==WIFI_STATION)?"STA":"AP"));
    display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
    display.drawString(64, 54, "IP: " + IP.toString());
    display.display();
  }
  if (!MDNS.begin(cfg.wifi_hostname)) {
    info("MDNS responder failed to start\n");
  }

    // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_index_html_start, data_index_html_end - data_index_html_start - 1);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", data_script_js_start, data_script_js_end - data_script_js_start - 1);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    String output = get_settings();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  server.on("/settings.json", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204, "text/html");
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    response->addHeader("Access-control-Allow-Credentials", "false");
    response->addHeader("Access-control-Allow-Headers", "x-requested-with");
    response->addHeader("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type");

    request->send(response);
  });
  server.on("/settings.json", HTTP_POST, [](AsyncWebServerRequest *request) {
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    DynamicJsonDocument json(1024);
    DeserializationError error = deserializeJson(json, data);
    info("/settings.json: post settings (%d)\n", error);
    if (error || !parse_settings(json)) {
      request->send(501, "text/plain", "deserializeJson failed");
    } else {
      String output = get_settings();
      AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      info("/settings.json: post settings done\n");
    }
  });
  server.on("/reboot", [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Connection", "close");
    request->send(response);
    EEPROM.commit();
    sleep(1);
    ESP.restart();
  });
  server.on("/factoryreset", [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    cfg.version = 0xff;
    write_config();
    sleep(1);
    ESP.restart();
  });

  server.on("/page", page);


  server.onNotFound(handleNotFound);
  server.begin();

  int state = fsk.beginFSK(cfg.tx_frequency, 0.622, cfg.tx_deviation, 10, cfg.tx_power, cfg.tx_current_limit, 0, false);
  state |= fsk.setDCFree(SX127X_DC_FREE_MANCHESTER);
  state |= fsk.setCRC(false);
  if (state != ERR_NONE) {
    info("beginFSK failed, code %d\n", state);
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
    info("RESET Config\n");
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
  check_display();
  check_buttons();

  if(cfg.ota_path[0] && WiFi.getMode() == WIFI_MODE_STA && WiFi.status() == WL_CONNECTED) {
    updater->CheckAndUpdate();
  }
}
