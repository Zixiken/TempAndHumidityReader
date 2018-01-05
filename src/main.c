#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "libnokiadisplay.h"

#define ADDRESS 0x44

uint8_t error = 0;

int crc(uint16_t data, uint8_t checksum) {
  static const uint8_t POLY = 0x31;

  uint8_t remaining, thisChecksum = 0xFF ^ ((uint8_t)(data >> 8));

  for(remaining = 8; remaining; remaining--)
    thisChecksum = (thisChecksum & 0x80) ?
        (thisChecksum << 1) ^ POLY :
        (thisChecksum << 1);

  thisChecksum ^= (uint8_t)data;
  for(remaining = 8; remaining; remaining--)
    thisChecksum = (thisChecksum & 0x80) ?
        (thisChecksum << 1) ^ POLY :
        (thisChecksum << 1);

  return thisChecksum == checksum;
}

int readDataPair(uint16_t * temp, uint16_t * humidity) {
  uint8_t tempChecksum, humidityChecksum;
  TWBR = 8;

  //Start
  TWCR = 0xA4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x08) {
    error = 1;
    return 0; //Check for successful start condition
  }

  //SLA+W
  TWDR = ADDRESS << 1;
  TWCR = 0x84;
  while(!(TWCR & 0x80));
  if(TWSR == 0x20) TWCR = 0x94; //Make sure to send stop if slave NACK'd
  if(TWSR != 0x18) {
    error = 2;
    return 0; //Check for slave ACK
  }

  //Command MSB
  TWDR = 0x2C;
  TWCR = 0x84;
  while(!(TWCR & 0x80));
  if(TWSR == 0x30) TWCR = 0x94; //Make sure to send stop if slave NACK'd
  if(TWSR != 0x28) {
    error = 3;
    return 0; //Check for slave ACK
  }

  //Command LSB
  TWDR = 0x06;
  TWCR = 0x84;
  while(!(TWCR & 0x80));
  if(TWSR == 0x30) TWCR = 0x94; //Make sure to send stop if slave NACK'd
  if(TWSR != 0x28) {
    error = 4;
    return 0; //Check for slave ACK
  }

  //Stop, then Start
  TWCR = 0xB4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x08) {
    error = 5;
    return 0; //Check for successful start condition
  }

  //SLA+R
  TWDR = (ADDRESS << 1) + 1;
  TWCR = 0x84;
  while(!(TWCR & 0x80));
  if(TWSR == 0x48) TWCR = 0x94; //Make sure to send stop if slave NACK'd
  if(TWSR != 0x40) {
    error = 6;
    return 0; //Check for slave ACK
  }

  //Temp MSB
  TWCR = 0xC4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x50) {
    error = 7;
    return 0; //Check for arbitration loss
  }
  *temp = ((uint16_t)TWDR) << 8;

  //Temp LSB
  TWCR = 0xC4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x50) {
    error = 8;
    return 0; //Check for arbitration loss
  }
  *temp += TWDR;

  //Temp CRC
  TWCR = 0xC4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x50) {
    error = 9;
    return 0; //Check for arbitration loss
  }
  tempChecksum = TWDR;

  //Humidity MSB
  TWCR = 0xC4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x50) {
    error = 10;
    return 0; //Check for arbitration loss
  }
  *humidity = ((uint16_t)TWDR) << 8;

  //Humidity LSB
  TWCR = 0xC4;
  while(!(TWCR & 0x80));
  if(TWSR != 0x50) {
    error = 11;
    return 0; //Check for arbitration loss
  }
  *humidity += TWDR;

  //Humidity CRC
  TWCR = 0x84;
  while(!(TWCR & 0x80));
  if(TWSR != 0x58) {
    error = 12;
    return 0; //Check for arbitration loss
  }
  humidityChecksum = TWDR;

  //Stop
  TWCR = 0x94;

  //Check CRC's
  return crc(*humidity, humidityChecksum) && crc(*temp, tempChecksum);
}

int main() {
  uint16_t temp, humidity, decimalPart;
  float result;
  int8_t intPart;

  static char buf[20];
  static float humidityConstant = 0.00152590218967; // 100 / (2^16 - 1)
  static float tempConstant = 0.00480659189746; // 315 / (2^16 - 1)

  DDRB = 0x70;
  DDRH = 0x60;

  initController(&PORTB, 6, &PORTB, 5, &PORTB, 4, &PORTH, 6, &PORTH, 5);
  setExtendedRegisters(4, 0x20, 0);
  clear();
  setDisplayMode(DISPLAY_MODE_NORMAL);
  setPowerMode(0);

  while(1) {
    if(!readDataPair(&temp, &humidity)) {
      sprintf(buf, "Error: %d", error);
      drawText(0, 0, buf, 1);
      sprintf(buf, "%x", TWSR);
      drawText(0, 8, buf, 1);
    } else {
      drawText(0, 0, "Temperature:", 1);

      result = (float)temp * tempConstant - 49;
      intPart = (int8_t)result;
      decimalPart = (int)((result - intPart) * 1000);
      sprintf(buf, "%d.%u", intPart, decimalPart);
      drawText(0, 8, buf, 1);

      drawText(0, 16, "Humidity:", 1);

      result = (float)humidity * humidityConstant;
      intPart = (int8_t)result;
      decimalPart = (int)((result - intPart) * 1000);
      sprintf(buf, "%d.%u", intPart, decimalPart);
      drawText(0, 24, buf, 1);
    }

    _delay_ms(1000);
  }
}
