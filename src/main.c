#include <avr/io.h>
#include <libnokiadisplay.h>

int crc(uint8_t data0, uint8_t data1, uint8_t checksum) {
  static const uint8_t POLY = 0x31;

  uint8_t remaining, thisChecksum = 0xFF ^ data0;

  for(remaining = 8; remaining; remaining--)
    thisChecksum = (thisChecksum & 0x80) ?
        (thisChecksum << 1) ^ POLY :
        (thisChecksum << 1);

  thisChecksum ^= data1;
  for(remaining = 8; remaining; remaining--)
    thisChecksum = (thisChecksum & 0x80) ?
        (thisChecksum << 1) ^ POLY :
        (thisChecksum << 1);

  return thisChecksum == checksum;
}

int main() {
  DDRB = 0x70;
  DDRH = 0x60;

  initController(&PORTB, 6, &PORTB, 5, &PORTB, 4, &PORTH, 6, &PORTH, 5);
  setExtendedRegisters(4, 0x2f, 0);
  clear();
  setDisplayMode(DISPLAY_MODE_NORMAL);
  setPowerMode(0);

  if(crc(0xDE, 0xAD, 0x98)) drawText(0, 0, "true", 0);

  while(1);
}
