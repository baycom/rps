#include "main.h"

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static AsyncEventSource events("/events");

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

static void page(AsyncWebServerRequest *request)
{
  int pager_number = -1;
  float tx_frequency = -1;
  float tx_deviation = -1;
  int alert_type = -1;
  int restaurant_id = -1;
  int system_id = -1;
 
  bool force = false;
  bool reprog = false;
  byte mode = cfg.default_mode;
  int pocsag_baud = cfg.pocsag_baud;
  int tx_power = cfg.tx_power;
  func_t pocsag_telegram_type = FUNC_BEEP;
  String message;
  bool cancel = false;
  int multi_pager_types = cfg.multi_pager_types;
  
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
  if (request->hasArg("cancel")) {
    cancel=request->arg("cancel").toInt();
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
  if (request->hasArg("multi_pager_types")) {
    multi_pager_types = request->arg("multi_pager_types").toInt();
  }
 
  if (pager_number > 0 || force) {
    String str="mode: "+String(mode)+"\ntx_power: "+String(tx_power)+"\ntx_frequency: "+String(tx_frequency,5)+
               "\ntx_deviation: "+String(tx_deviation)+"\npocsag_baud: "+String(pocsag_baud)+ 
               "\nrestaurant_id: "+String(restaurant_id)+ "\nsystem_id: "+String(system_id) +
               "\nreprogram:"+String(reprog)+"\ncancel:"+String(cancel)+ 
               "\npager_number: "+String(pager_number)+ "\nalert_type: "+String(alert_type)+ 
               "\npocsag_telegram_type: "+String(pocsag_telegram_type)+"\nmessage: "+message+
               "\nmulti_pager_types: "+String(multi_pager_types)+
               "\n" ;

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", str);

    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    if(multi_pager_types) {
      if(multi_pager_types & 1) {
        call_pager(0, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, reprog, pocsag_telegram_type, message.c_str(), cancel);
      }
      if(multi_pager_types & 2) {
        call_pager(1, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, reprog, pocsag_telegram_type, message.c_str(), cancel);
      }
      if(multi_pager_types & 4) {
        call_pager(2, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, reprog, pocsag_telegram_type, message.c_str(), cancel);
      }
      if(multi_pager_types & 8) {
        call_pager(3, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, reprog, pocsag_telegram_type, message.c_str(), cancel);
      }

    } else {
      call_pager(mode, tx_power, tx_frequency, tx_deviation, pocsag_baud, restaurant_id, system_id, pager_number, alert_type, reprog, pocsag_telegram_type, message.c_str(), cancel);
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

void webserver_setup() {

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
}