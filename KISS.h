#ifndef _KISS_H_
#define _KISS_H_

#include <HamShield.h>
#include "AFSK.h"

#define KISS_FEND 0xC0
#define KISS_FESC 0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

class KISS {
public:
  KISS(Stream *_io, HamShield *h, DDS *d) : io(_io), radio(h), dds(d) {}
  bool read();
  void writePacket(AFSK::Packet *);
  void loop();
  inline void isr() {
    static uint8_t tcnt = 0;
    TIFR1 = _BV(ICF1); // Clear the timer flag
    dds->clockTick();
    if(++tcnt == (DDS_REFCLK_DEFAULT/9600)) {
      //PORTD |= _BV(2); // Diagnostic pin (D2)
      radio->afsk.timer();
      tcnt = 0;
    }
    //PORTD &= ~(_BV(2));
  }
private:
  Stream *io;
  HamShield *radio;
  DDS *dds;
};

#endif /* _KISS_H_ */
