# Restaurant Paging Service (RPS)
Restaurant Paging Service based on ESP32 and SX1278 using the Long Range Systems Protocol or plain POCSAG

A video explaining what this thing is good for can be found here: https://youtu.be/AoRPzNYkjQ0

This code is based on the Arduino framework and runs on a Heltec / TTGO
SX1278 433Mhz LoRa module (for example from here
https://www.ebay.de/itm/SX1278-LoRa-ESP32-0-96-blau-ESP8266-OLED-Display-Bluetooth-WIFI-Lora-Kit-32/152891174278)

Besides the web frontend there is a HTTP based API at /page with these parameters:
- mode=[0|1] 
  switches between LRS (0) and POCSAG (1) protocol
- tx_power=[0-20]
  sets the transmission power in dBm
- tx_frequency=[137.00000-525.00000]
  sets the transmission frequency in Mhz
- tx_deviation=[0.0-25.0]
  sets the FM deviation in Khz
- pocsag_baud=[512,1200,2400]
  sets the baudrate for POCSAG mode
- restaurant_id=[0-255]
  sets the restaurant id for LRS mode
- system_id=[0-255]
  sets the system id for LRS mode
- pager_number=[0-2097148]
  sets the pager address for LRS (0-4095) or POCSAG (0-2097151 / 21bit), in LRS mode 0 means all pages and requires the parameter force set to 1
- alert_type=[0-255]
  sets the alert type for LRS or the function code for POCSAG (0-2)
- pocsag_telegram_type=[0-2]
  sets the POCSAG telegram type 0:Beep, 1:Numerical, 2:Alpha
- message=<text>
  sets the numerical or alpha message

Example:

```
http://<rps>/page?force=1&pager_number=17&alert_type=1&mode=1&pocsag_baud=1200&pocsag_telegram_type=2&message=this%20is%20a%20test
```

When using POCSAG with the LRS alpha numeric pagers the pager number (POCSAG address) is calculcated like that:

```
if(pager_num<8192) 
    return 700000+pager_num*8
else
    return 700000+(pager_num-8192)*8
```

In POCSAG-Mode programming pagers follows this scheme:

```
- Set welcome message: 
  Message 1: Address 100008, Function 3, Beep
  Message 2: Address 100000, Function 3, Alpha: [W<text>]
  Example: [WTest]

- Set addresses:
  Message 1: Address 100008, Function 3, Beep
  Message 2: Address 100000, Function 3, Alpha: [C10<single-address>]
  Example: [C10700008] = 1
  Message 3: Address 100008, Function 3, Beep
  Message 4: Address 100000, Function 3, Alpha: [C20<all-address>]
  Example: [C20707288] = 911
  Message 5: Address 100008, Function 3, Beep
  Message 6: Address 100000, Function 3, Alpha: [C30<system-address>]
  Example: [C30700000] = 0
  
```
