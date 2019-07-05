#include "SX1277.h"

SX1277::SX1277(Module* mod) : SX1278(mod) {
  
}

int16_t SX1277::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint8_t currentLimit, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(SX1278_CHIP_VERSION, syncWord, currentLimit, preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure settings not accessible by API
  state = config();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setBandwidth(bw);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setSpreadingFactor(sf);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setCodingRate(cr);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setGain(gain);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(state);
}

int16_t SX1277::setFrequency(float freq) {
  // check frequency range
  if((freq < 137.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // SX1276/77/78 Errata fixes
  if(getActiveModem() == SX127X_LORA) {
    // sensitivity optimization for 500kHz bandwidth
    // see SX1276/77/78 Errata, section 2.1 for details
    if(abs(_bw - 500.0) <= 0.001) {
      if((freq >= 862.0) && (freq <= 1020.0)) {
        _mod->SPIwriteRegister(0x36, 0x02);
        _mod->SPIwriteRegister(0x3a, 0x64);
      } else if((freq >= 410.0) && (freq <= 525.0)) {
        _mod->SPIwriteRegister(0x36, 0x02);
        _mod->SPIwriteRegister(0x3a, 0x7F);
      }
    }
    
    // mitigation of receiver spurious response
    // see SX1276/77/78 Errata, section 2.3 for details
    if(abs(_bw - 7.8) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x48);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 7.8;
    } else if(abs(_bw - 10.4) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 10.4;
    } else if(abs(_bw - 15.6) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 15.6;
    } else if(abs(_bw - 20.8) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 20.8;
    } else if(abs(_bw - 31.25) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 31.25;
    } else if(abs(_bw - 41.7) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 41.7;
    } else if(abs(_bw - 62.5) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 125.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 250.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 500.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b1000000, 7, 7);
    }
  }
  
  // set frequency and if successful, save the new setting
  return(SX127x::setFrequencyRaw(freq));
}

int16_t SX1277::setSpreadingFactor(uint8_t sf) {
  uint8_t newSpreadingFactor;
  
  // check allowed spreading factor values
  switch(sf) {
    case 6:
      newSpreadingFactor = SX127X_SF_6;
      break;
    case 7:
      newSpreadingFactor = SX127X_SF_7;
      break;
    case 8:
      newSpreadingFactor = SX127X_SF_8;
      break;
    case 9:
      newSpreadingFactor = SX127X_SF_9;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  // set spreading factor and if successful, save the new setting
  int16_t state = SX1278::setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;
  }
  
  return(state);
}
