# Restaurant Paging Service (RPS)
Restaurant Paging Service based on ESP32 and SX1276/SX1278 using the Long Range Systems Protocol, plain POCSAG, RETEKESS OOK T112 (24bit), RETEKESS OOK TD161 (33bit) or RETEKESS 2-FSK T164.

A video explaining what this thing is good for can be found here: https://youtu.be/AoRPzNYkjQ0
[![RPS in action](https://img.youtube.com/vi/AoRPzNYkjQ0/0.jpg)](https://www.youtube.com/watch?v=AoRPzNYkjQ0)

This code is based on the Arduino framework and runs on a Heltec / TTGO
SX1278 433Mhz LoRa module (for example from here
https://www.ebay.de/itm/SX1278-LoRa-ESP32-0-96-blau-ESP8266-OLED-Display-Bluetooth-WIFI-Lora-Kit-32/152891174278)

## Initial Setup

By default RPS sets up an accesspoint (SSID named 'RPS', No Encryption). Connect to the network and enter http://192.168.4.1 as URL into your web browser. Click on the red X to close the number pad. Open the configuration form by clicking on the button 'Settings'. The form is divided into three framesets 'WIFI', 'Transmitter' and 'Paging Service'. Set whatever you need to change. After that press 'Save & Restart'. 

Currently frequencies have to be entered with an offset of -9,46kHz (see section 'Troubleshooting').

![Settings Dialog](https://github.com/baycom/rps/raw/master/settings.jpg)


## Besides the web frontend there is a HTTP based API at /page with these parameters:
- mode=[0|1|2|3|4]
  switches between LRS (0), POCSAG (1), RETEKESS OOK T112 (2), RETEKESS FSK TD164 (3) and RETEKESS OOK TD161 (4) protocol
- tx_power=[0-20]
  sets the transmission power in dBm
- tx_frequency=[137.00000-525.00000]
  sets the transmission frequency in Mhz
- tx_deviation=[0.0-25.0]
  sets the FSK deviation in Khz
- pocsag_tx_frequency=[137.00000-525.00000]
  sets the transmission frequency in Mhz
- pocsag_tx_deviation=[0.0-25.0]
  sets the FSK deviation in Khz
- retekess_tx_frequency=[137.00000-525.00000]
  sets the transmission frequency in Mhz
- retekess_tx_deviation=[0.0-25.0]
  sets the FSK deviation in Khz
- pocsag_baud=[512,1200,2400]
  sets the baudrate for POCSAG mode
- restaurant_id=[0-255]
  sets the restaurant id for LRS mode
- system_id=[0-255]
  sets the system id for LRS mode
- retekess_system_id=[0-8191]
  sets the system id for RETEKESS mode
- pager_number=[0-2097151]
  sets the pager address for LRS (0-4095) or POCSAG (0-2097151 / 21bit), in LRS mode 0 means all pagers and requires the parameter force being set to 1
- alert_type=[0-255]
  sets the alert type for LRS or the function code for POCSAG (0-3) or the function number in RETEKESS TD161 mode (
  0: paging mode, 1: programming mode, 2: alert mode (A001-A009), 3: alert time (1-999s), 4: alert wait repeat (1-999s). Programming mode requires an initial call to pager_number 0 followed by the desired pager_number). In TD164 mode 1 and calling 999 means to mute all pagers, calling 000 means to unmute all pagers.
- reprogram=[0|1]
  programs a pager in LRS mode to a new address. Vibrating can be switched off or on by setting alert_type to 0 or 1. In FSK TD164 mode this means to sent out programming datagrams. First call 999 to switch pages on the charge station into programming mode, then call the desired number the pager should get when taking it from the charging station. 
- pocsag_telegram_type=[0-2]
  sets the POCSAG telegram type 0:Beep, 1:Numerical, 2:Alpha
- message=<text>
  sets the numerical or alpha message
- multi_pager_types=[0-15]
  allows multiple paging modes to be used within one call via web-frontend. Example: 3 means LRS(1) + POCSAG(2), RETEKESS OOK would be 4, REKEKESS FSK is 8, RETEKESS OOK new is 16 

Example:

```
http://<rps>/page?force=1&pager_number=17&alert_type=1&mode=1&pocsag_baud=1200&pocsag_telegram_type=2&message=this%20is%20a%20test
```

## When using POCSAG with the LRS alpha numeric pagers the pager number (POCSAG address) is calculcated like that:

```
POCSAG address = (pager_num & 0x1fff) * 8 + 700000
```

## In POCSAG-Mode programming pagers follows this scheme:

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
## Factory Reset

A factory reset can be issued by pressing the 'PRG' button (GPIO0) for more than 5 seconds.

## Over The Air (OTA) Updates 

There is a shell script called 'deploy' which generates a suitable configuration file for OTA. For proper OTA operation you have to increase the 'VERSION_NUMBER' in version.h for every new version. Set your update server location via web frontend at 'OTA URL'.

## Building the code

This project uses Microsoft Visual Studio Code with PlattformIO. If you make changes to the web content in the data folder you have to run the script 'mkinc' in the include folder. The project depends on these libraries:

- LoRaLib - this library has been modified to enable manchester coding and switching off sync words
- esp8266-oled-ssd1306 - the OLED display driver library
- EOTAUpdate - the OTA library
- ArduinoJson - a library to handle JSON data

## Troubleshooting

- If the device cannot connect to a WiFi network in station mode within the first 10 seconds it will switch over to AP mode with SSID 'RPS' and no secret.
- In case the display shows 'SX127X FAIL' the transceiver chip is broken or unusable.
- In case some pagers are not working reliable adjust the transmitter frequency. For example if 446,15625Mhz is required, the frequency to enter is 446,14679 (offset of -9,46kHz). This might be a bug of cheap and/or poor SX127X PCB designs.
- In case POCSAG does not work (transmitter has no modulation) make sure SX127X PIN DIO2 is connected to ESP32 GPIO 32. On Heltec V2 boards DIO2 is connected to GPIO 34, which does not work out of the box. To fix this simply connect GPIO 32 and GPIO 34 with a short piece of wire.
- RETEKESS pagers seem to exist in different versions having the same model number. This means that you have to read the specification carefully. An indicator which model you have is the modulation system (OOK or FSK). Further RETEKESS seems to implement a new frame structure, bitrate, etc. with almost every model. This makes changes to the RPS code necessary to make it work again.
