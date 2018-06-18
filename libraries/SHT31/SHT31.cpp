#include "SHT31.h"

SHT31::SHT31() {
}

boolean SHT31::begin(uint8_t i2caddr) {
  Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_EXT, I2C_RATE_100);
  // Required to fix issue with i2c_t3
  // -----------------------------------
  CORE_PIN29_CONFIG = PORT_PCR_MUX(2);
  CORE_PIN30_CONFIG = PORT_PCR_MUX(2);
  // -----------------------------------
  _i2caddr = i2caddr;
  reset();
  //return (readStatus() == 0x40);
  return true;
}

float SHT31::getTemperature(void) {
  if (! getTempHum()) return NAN;
  return temp;
}


float SHT31::getHumidity(void) {
  if (! getTempHum()) return NAN;
  return humidity;
}

SHT31_Reading SHT31::getReading(void)
{
    SHT31_Reading reading = {NAN, NAN};
    if (getTempHum()) 
    {
        reading.temperature = temp;
        reading.humidity = humidity;
    }
    return reading;
}

uint16_t SHT31::readStatus(void) {  
}

void SHT31::reset(void) {
  writeCommand(SHT31_SOFTRESET);
  delay(10);
}

void SHT31::heater(boolean h) {
  if (h)
    writeCommand(SHT31_HEATEREN);
  else
    writeCommand(SHT31_HEATERDIS);
}

uint8_t SHT31::crc8(const uint8_t *data, int len) {
  const uint8_t POLYNOMIAL(0x31);
  uint8_t crc(0xFF);
  
  for ( int j = len; j; --j ) {
      crc ^= *data++;

      for ( int i = 8; i; --i ) {
	crc = ( crc & 0x80 )
	  ? (crc << 1) ^ POLYNOMIAL
	  : (crc << 1);
      }
  }
  return crc; 
}


boolean SHT31::getTempHum(void) {
  uint8_t readbuffer[6];

  writeCommand(SHT31_MEAS_HIGHREP);
  
  delay(50);
  Wire1.requestFrom(_i2caddr, (uint8_t)6);
  if (Wire1.available() != 6) 
    return false;
  for (uint8_t i=0; i<6; i++) {
    readbuffer[i] = Wire1.read();
  }
  uint16_t ST, SRH;
  ST = readbuffer[0];
  ST <<= 8;
  ST |= readbuffer[1];

  if (readbuffer[2] != crc8(readbuffer, 2)) return false;

  SRH = readbuffer[3];
  SRH <<= 8;
  SRH |= readbuffer[4];

  if (readbuffer[5] != crc8(readbuffer+3, 2)) return false;
 
  double stemp = ST;
  stemp *= 175;
  stemp /= 0xffff;
  stemp = -45 + stemp;
  temp = stemp;
  
  double shum = SRH;
  shum *= 100;
  shum /= 0xFFFF;
  
  humidity = shum;
  
  return true;
}

void SHT31::writeCommand(uint16_t cmd) {
  Wire1.beginTransmission(_i2caddr);
  Wire1.write(cmd >> 8);
  Wire1.write(cmd & 0xFF);
  Wire1.endTransmission();      
}
