/*
   LoRaLib FSK Modem Example

   This example shows how to use FSK modem in SX127x chips.
   
   NOTE: The sketch below is just a guide on how to use
         FSK modem, so this code should not be run directly!
         Instead, modify the other examples to use FSK
         modem and use the appropriate configuration
         methods.
   
   For more detailed information, see the LoRaLib Wiki
   https://github.com/jgromes/LoRaLib/wiki

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/LoRaLib/

   For more information on FSK modem, see
   https://github.com/jgromes/LoRaLib/wiki/FSK-Modem
*/

// include the library
#include <LoRaLib.h>

// create instance of LoRa class using SX1278 module
// this pinout corresponds to RadioShield
// https://github.com/jgromes/RadioShield
// NSS pin:   10 (4 on ESP32/ESP8266 boards)
// DIO0 pin:  2
// DIO1 pin:  3
SX1278 fsk = new LoRa;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 FSK modem with default settings
  Serial.print(F("Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                125.0 kHz
  // output power:                13 dBm
  // current limit:               100 mA
  // data shaping:                Gaussian, BT = 0.3
  // sync word:                   0x2D  0x01
  // OOK modulation:              false
  int state = fsk.beginFSK();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, you can switch between LoRa and FSK modes
  //
  // lora.begin()       start LoRa mode (and disable FSK)
  // lora.beginFSK()    start FSK mode (and disable LoRa)

  // the following settings can also 
  // be modified at run-time
  state = fsk.setFrequency(433.5);
  state = fsk.setBitRate(100.0);
  state = fsk.setFrequencyDeviation(10.0);
  state = fsk.setRxBandwidth(250.0);
  state = fsk.setOutputPower(10.0);
  state = fsk.setCurrentLimit(100);
  state = fsk.setDataShaping(0.5);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67, 
                        0x89, 0xAB, 0xCD, 0xEF};
  state = fsk.setSyncWord(syncWord, 8);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true);
  }

  // FSK modulation can be changed to OOK
  // NOTE: When using OOK, the maximum bit rate is only 32.768 kbps!
  //       Also, data shaping changes from Gaussian filter to
  //       simple filter with cutoff frequency. Make sure to call
  //       setDataShapingOOK() to set the correct shaping!
  state = fsk.setOOK(true);
  state = fsk.setDataShapingOOK(1);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to change modulation, code "));
    Serial.println(state);
    while (true);
  }

  #warning "This sketch is just an API guide! Read the note at line 6."
}

void loop() {
  // FSK modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions
  // NOTE: FSK modem maximum packet length is 63 bytes!
  
  // transmit FSK packet
  int state = fsk.transmit("Hello World!");
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                      0x78, 0xAB, 0xCD, 0xEF};
    int state = lora.transmit(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("Packet transmitted successfully!"));
  } else if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F("Packet too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F("Timed out while transmitting!"));
  } else {
    Serial.println(F("Failed to transmit packet, code "));
    Serial.println(state);
  }

  // receive FSK packet
  String str;
  state = fsk.receive(str);
  /*
    byte byteArr[8];
    int state = lora.receive(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("Received packet!"));
    Serial.print(F("Data:\t"));
    Serial.println(str);
  } else if (state == ERR_RX_TIMEOUT) {
    Serial.println(F("Timed out while waiting for packet!"));
  } else {
    Serial.println(F("Failed to receive packet, code "));
    Serial.println(state);
  }

  // FSK modem has built-in address filtering system
  // it can be enabled by setting node address, broadcast
  // address, or both
  //
  // to transmit packet to a particular address, 
  // use the following methods:
  //
  // fsk.transmit("Hello World!", address);
  // fsk.startTransmit("Hello World!", address);

  // set node address to 0x02
  state = fsk.setNodeAddress(0x02);
  // set broadcast address to 0xFF
  state = fsk.setBroadcastAddress(0xFF);
  if (state != ERR_NONE) {
    Serial.println(F("Unable to set address filter, code "));
    Serial.println(state);
  }

  // address filtering can also be disabled
  // NOTE: calling this method will also erase previously set
  // node and broadcast address
  /*
    state = fsk.disableAddressFiltering();
    if (state != ERR_NONE) {
      Serial.println(F("Unable to remove address filter, code "));
    }
  */

  // FSK modem supports direct data transmission
  // in this mode, SX127x directly transmits any data
  // sent to DIO1 (data) and DIO2 (clock)

  // activate direct mode transmitter
  state = fsk.transmitDirect();
  if (state != ERR_NONE) {
    Serial.println(F("Unable to start direct transmission mode, code "));
    Serial.println(state);
  }

  // using the direct mode, it is possible to transmit
  // FM tones with Arduino tone() function
  
  // it is recommended to set data shaping to 0
  // (no shaping) when transmitting audio
  state = fsk.setDataShaping(0.0);
  if (state != ERR_NONE) {
    Serial.println(F("Unable to set data shaping, code "));
    Serial.println(state);
  }

  // tone() function is not available on ESP32 and Arduino Due
  #if !defined(ESP32) && !defined(_VARIANT_ARDUINO_DUE_X_)
  // transmit FM tone at 1000 Hz for 1 second
  // (DIO2 is connected to Arduino pin 4)
  tone(4, 1000);
  delay(1000);
  // transmit FM note at 500 Hz for 1 second
  tone(4, 500);
  delay(1000);
  #endif

  // NOTE: after calling transmitDirect(), SX127x will start
  // transmitting immediately! This signal can jam other
  // devices at the same frequency, it is up to the user
  // to disable it with standby() method!

  // direct mode transmissions can also be received
  // as bit stream on DIO1 (data) and DIO2 (clock)
  state = fsk.receiveDirect();
  if (state != ERR_NONE) {
    Serial.println(F("Unable to start direct reception mode, code "));
    Serial.println(state);
  }
  
  // NOTE: you will not be able to send or receive packets
  // while direct mode is active! to deactivate it, call method
  // fsk.packetMode()
}
