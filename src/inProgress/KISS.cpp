#include <HamShield.h>
#include "AFSK.h"
#include "KISS.h"

//AFSK::Packet kissPacket;
bool inFrame = false;
uint8_t kissBuffer[PACKET_MAX_LEN];
uint16_t kissLen = 0;

// Inside the KISS loop, we basically wait for data to come in from the
// KISS equipment, and look if we have anything to relay along
void KISS::loop() {
  static bool currentlySending = false;
  if(radio->afsk.decoder.read() || radio->afsk.rxPacketCount()) {
     // A true return means something was put onto the packet FIFO
     // If we actually have data packets in the buffer, process them all now
     while(radio->afsk.rxPacketCount()) {
       AFSK::Packet *packet = radio->afsk.getRXPacket();
       if(packet) {
         writePacket(packet);
         AFSK::PacketBuffer::freePacket(packet);
       }
     }
   }
   // Check if we have incoming data to turn into a packet
   while(io->available()) {
     uint8_t c = (uint8_t)io->read();
     if(c == KISS_FEND) {
       if(inFrame && kissLen > 0) {
         int i;
         AFSK::Packet *packet = AFSK::PacketBuffer::makePacket(PACKET_MAX_LEN);
         packet->start();
         for(i = 0; i < kissLen; i++) {
           packet->appendFCS(kissBuffer[i]);
         }
         packet->finish();
         radio->afsk.encoder.putPacket(packet);
       }
       kissLen = 0;
       inFrame = false;
     }
     // We're inside the boundaries of a FEND
     if(inFrame) {
       // Unescape the incoming data
       if(c == KISS_FESC) {
         c = io->read();
         if(c == KISS_TFESC) {
           c = KISS_FESC;
         } else {
           c = KISS_FEND;
         }
       }
       kissBuffer[kissLen++] = c;
     }
     if(kissLen == 0 && c != KISS_FEND) {
       if((c & 0xf) == 0) // First byte<3:0> should be a 0, otherwise we're having options
         inFrame = true;
     }
   }
   if(radio->afsk.txReady()) {
     radio->setModeTransmit();
     currentlySending = true;
     if(!radio->afsk.txStart()) { // Unable to start for some reason
       radio->setModeReceive();
       currentlySending = false;
     }
   }
   if(currentlySending && radio->afsk.encoder.isDone()) {
    radio->setModeReceive();
    currentlySending = false;
  }
}

void KISS::writePacket(AFSK::Packet *p) {
  int i;
  io->write(KISS_FEND);
  io->write((uint8_t)0); // Host to TNC port identifier
  for(i = 0; i < p->len-2; i++) {
    char c = p->getByte(i);
    if(c == KISS_FEND || c == KISS_FESC) {
      io->write(KISS_FESC);
      io->write((c==KISS_FEND?KISS_TFEND:KISS_TFESC));
    } else {
      io->write(c);
    }
  }
  io->write(KISS_FEND);
}
