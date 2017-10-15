#include <avr/io.h>
#include <libnokiadisplay.h>

int main() {
  DDRB = 0x80;
  initPorts(&PORTB, 7, &PORTB, 0, &PORTB, 0, &PORTB, 0);
}
