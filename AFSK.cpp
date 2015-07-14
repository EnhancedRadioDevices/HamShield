#include <Arduino.h>
#include "HamShield.h"
#include "SimpleFIFO.h"
#include <util/atomic.h>

#define PHASE_BIT 8
#define PHASE_INC 1

#define PHASE_MAX (SAMPLEPERBIT * PHASE_BIT)
#define PHASE_THRES (PHASE_MAX / 2)

#define BIT_DIFFER(bitline1, bitline2) (((bitline1) ^ (bitline2)) & 0x01)
#define EDGE_FOUND(bitline)            BIT_DIFFER((bitline), (bitline) >> 1)

#define PPOOL_SIZE 2

#define AFSK_SPACE 2200
#define AFSK_MARK  1200

// Timers
volatile unsigned long lastTx = 0;
volatile unsigned long lastTxEnd = 0;
volatile unsigned long lastRx = 0;

#define T_BIT ((unsigned int)(SAMPLERATE/BITRATE))

#ifdef PACKET_PREALLOCATE
SimpleFIFO<AFSK::Packet *,PPOOL_SIZE> preallocPool;
AFSK::Packet preallocPackets[PPOOL_SIZE];
#endif

void AFSK::Encoder::process() {
  // We're on the start of a byte position, so fetch one
  if(bitPosition == 0) {
    if(preamble) { // Still in preamble
      currentByte = HDLC_PREAMBLE;
      --preamble; // Decrement by one
    } else {
      if(!packet) { // We aren't on a packet, grab one
        // Unless we already sent enough
        if(maxTx-- == 0) {
          stop();
          lastTxEnd = millis();
          return;
        }
        packet = pBuf.getPacket();
        if(!packet) { // There actually weren't any
          stop(); // Stop transmitting and return
          lastTxEnd = millis();
          return;
        }
        lastTx = millis();
        currentBytePos = 0;
        nextByte = HDLC_FRAME; // Our next output should be a frame boundary
        hdlc = true;
      }
  
      // We ran out of actual data, provide an HDLC frame (idle)
      if(currentBytePos == packet->len && nextByte == 0) {
        // We also get here if nextByte isn't set, to handle empty frames
        pBuf.freePacket(packet);
        packet = pBuf.getPacket(); // Get the next, if any
        //packet = NULL;
        currentBytePos = 0;
        nextByte = 0;
        currentByte = HDLC_FRAME;
        hdlc = true;
      } else {
        if(nextByte) {
          // We queued up something other than the actual stream to be sent next
          currentByte = nextByte;
          nextByte = 0;
        } else {
          // Get the next byte to send, but if it's an HDLC frame, escape it
          // and queue the real byte for the next cycle.
          currentByte = packet->getByte();
          if(currentByte == HDLC_FRAME) {
            nextByte = currentByte;
            currentByte = HDLC_ESCAPE;
          } else {
            currentBytePos++;
          }
          hdlc = false; // If we get here, it will be NRZI bit stuffed
        }
      }
    }
  }

  // Pickup the last bit
  currentBit = currentByte & 0x1;    

  if(lastZero == 5) {
    currentBit = 0; // Force a 0 bit output
  } else {
    currentByte >>= 1; // Bit shift it right, for the next round
    ++bitPosition; // Note our increase in position
  }

  // To handle NRZI 5 bit stuffing, count the bits
  if(!currentBit || hdlc)
    lastZero = 0;
  else
    ++lastZero;

  // NRZI and AFSK uses toggling 0s, "no change" on 1
  // So, if not a 1, toggle to the opposite tone
  if(!currentBit)
    currentTone = !currentTone;
  
  if(currentTone == 0) {
    PORTD |= _BV(7);
    dds->setFrequency(AFSK_SPACE);
  } else {
    PORTD &= ~_BV(7);
    dds->setFrequency(AFSK_MARK);
  }
}

bool AFSK::Encoder::start() {
  if(!done || sending) {
    return false;
  }
  
  if(randomWait > millis()) {
    return false;
  }
  
  // First real byte is a frame
  currentBit = 0;
  lastZero = 0;
  bitPosition = 0;
  //bitClock = 0;
  preamble = 0b110000; // 6.7ms each, 23 = 153ms
  done = false;
  hdlc = true;
  packet = 0x0; // No initial packet, find in the ISR
  currentBytePos = 0;
  maxTx = 3;
  sending = true;
  nextByte = 0;
  dds->setFrequency(0);
  dds->on();
  return true;
}

void AFSK::Encoder::stop() {
  randomWait = 0;
  sending = false;
  done = true;
  dds->setFrequency(0);
  dds->off();
}

AFSK::Decoder::Decoder() {
  // Initialize the sampler delay line (phase shift)
  //for(unsigned char i = 0; i < SAMPLEPERBIT/2; i++)
  //  delay_fifo.enqueue(0);
}

bool AFSK::HDLCDecode::hdlcParse(bool bit, SimpleFIFO<uint8_t,HAMSHIELD_AFSK_RX_FIFO_LEN> *fifo) {
  bool ret = true;
  
  demod_bits <<= 1;
  demod_bits |= bit ? 1 : 0;
  
  // Flag
  if(demod_bits == HDLC_FRAME) {
    fifo->enqueue(HDLC_FRAME);
    rxstart = true;
    currchar = 0;
    bit_idx = 0;
    return ret;
  }
  
  // Reset
  if((demod_bits & HDLC_RESET) == HDLC_RESET) {
    rxstart = false;
    lastRx = millis();
    return ret;
  }
  if(!rxstart) {
    return ret;
  }
  
  // Stuffed?
  if((demod_bits & 0x3f) == 0x3e)
    return ret;
  
  if(demod_bits & 0x01)
    currchar |= 0x80;
  
  if(++bit_idx >= 8) {
    if(currchar == HDLC_FRAME ||
      currchar == HDLC_RESET ||
      currchar == HDLC_ESCAPE) {
        fifo->enqueue(HDLC_ESCAPE);
      }
    fifo->enqueue(currchar & 0xff);
    currchar = 0;
    bit_idx = 0;
  } else {
    currchar >>= 1;
  }  

  return ret;
}

template <typename T, int size>
class FastRing {
private:
  T ring[size];
  uint8_t position;
public:
  FastRing(): position(0) {}
  inline void write(T value) {
    ring[(position++) & (size-1)] = value;
  }
  inline T read() const {
    return ring[position & (size-1)];
  }
  inline T readn(uint8_t n) const {
    return ring[(position + (~n+1)) & (size-1)];
  }
};
// Create a delay line that's half the length of the bit cycle (-90 degrees)
FastRing<uint8_t,(T_BIT/2)> delayLine;

// Handle the A/D converter interrupt (hopefully quickly :)
void AFSK::Decoder::process(int8_t curr_sample) {
  // Run the same through the phase multiplier and butterworth filter
  iir_x[0] = iir_x[1];
  iir_x[1] = ((int8_t)delayLine.read() * curr_sample) >> 2;
  iir_y[0] = iir_y[1];
  iir_y[1] = iir_x[0] + iir_x[1] + (iir_y[0] >> 1) + (iir_y[0]>>3) + (iir_y[0]>>5);
  
  // Place this ADC sample into the delay line
  delayLine.write(curr_sample);

  // Shift the bit into place based on the output of the discriminator
  sampled_bits <<= 1;
  sampled_bits |= (iir_y[1] > 0) ? 1 : 0;  
  
  // If we found a 0/1 transition, adjust phases to track
  if(EDGE_FOUND(sampled_bits)) {
    if(curr_phase < PHASE_THRES)
      curr_phase += PHASE_INC;
    else
      curr_phase -= PHASE_INC;
  }
  
  // Move ahead in phase
  curr_phase += PHASE_BIT;
  
  // If we've gone over the phase maximum, we should now have some data
  if(curr_phase >= PHASE_MAX) {
    curr_phase %= PHASE_MAX;
    found_bits <<= 1;
    
    // If we have 3 bits or more set, it's a positive bit
    register uint8_t bits = sampled_bits & 0x07;
    if(bits == 0x07 || bits == 0x06 || bits == 0x05 || bits == 0x03) {
      found_bits |= 1;
    }
    
    hdlc.hdlcParse(!EDGE_FOUND(found_bits), &rx_fifo); // Process it
  }
}
  
// This routine uses a pre-allocated Packet structure
// to save on the memory requirements of the stream data
bool AFSK::Decoder::read() {
  bool retVal = false;
  if(!currentPacket) { // We failed a prior memory allocation
    currentPacket = pBuf.makePacket(PACKET_MAX_LEN);
    if(!currentPacket) // Still nothing
      return false;
  }
  // While we have AFSK receive FIFO bytes...
  while(rx_fifo.count()) {
    // Grab the character
    char c = rx_fifo.dequeue();
    bool escaped = false;
    if(c == HDLC_ESCAPE) { // We received an escaped byte, mark it
      escaped = true;
      // Do we want to keep HDLC_ESCAPEs in the packet?
      //currentPacket->append(HDLC_ESCAPE); // Append without FCS
      c = rx_fifo.dequeue(); // Reset to the next character
    }
    
    // Append all the bytes
    // This will include unescaped HDLC_FRAME bytes
    if(c != HDLC_FRAME || escaped) // Append frame if it is escaped
      currentPacket->appendFCS(c); // Escaped characters and all else go into FCS
    
    if(currentPacket->len > PACKET_MAX_LEN) {
      // We've now gone too far and picked up far too many bytes
      // Cancel this frame, start back at the beginning
      currentPacket->clear();
      continue;
    }
    
    // We have a frame boundary, if it isn't escaped
    // If it's escaped, it was part of the data stream
    if(c == HDLC_FRAME && !escaped) {
      if(!currentPacket->len) {
        currentPacket->clear(); // There wasn't any data, restart stream
        continue;
      } else {
        // We have some bytes in stream, check it meets minimum payload length
        // Min payload is 1 (flag) + 14 (addressing) + 2 (control/PID) + 1 (flag)
        if(currentPacket->len >= 16) {          
          // We should end up here with a valid FCS due to the appendFCS
          if(currentPacket->crcOK()) { // Magic number for the CRC check passing
            // Valid frame, so, let's filter for control + PID
            // Maximum search distance is 71 bytes to end of the address fields
            // Skip the HDLC frame start
            bool filtered = false;
            for(unsigned char i = 0; i < (currentPacket->len<70?currentPacket->len:71); ++i) {
              if((currentPacket->getByte() & 0x1) == 0x1) { // Found a byte with LSB set
                // which marks the final address payload
                // next two bytes should be the control/PID
                //if(currentPacket->getByte() == 0x03 && currentPacket->getByte() == 0xf0) {
                  filtered = true;
                  break; // Found it
                //}
              }
            }
            
            if(!filtered) {
              // Frame wasn't one we care about, discard
              currentPacket->clear();
              continue;
            }
            
            // It's all done and formatted, ready to go
            currentPacket->ready = 1;
            if(!pBuf.putPacket(currentPacket)) // Put it in the receive FIFO
              pBuf.freePacket(currentPacket); // Out of FIFO space, so toss it
            
            // Allocate a new one of maximum length
            currentPacket = pBuf.makePacket(PACKET_MAX_LEN);
            retVal = true;
          }
        }
      }
      // Restart the stream
      currentPacket->clear();
    }
  }
  return retVal; // This is true if we parsed a packet in this flow
}
  
void AFSK::Decoder::start() {
  // Do this in start to allocate our first packet
  currentPacket = pBuf.makePacket(PACKET_MAX_LEN);
/*  ASSR &= ~(_BV(EXCLK) | _BV(AS2));

  // Do non-inverting PWM on pin OC2B (arduino pin 3) (p.159).
  // OC2A (arduino pin 11) stays in normal port operation:
  // COM2B1=1, COM2B0=0, COM2A1=0, COM2A0=0
  // Mode 1 - Phase correct PWM
  TCCR2A = (TCCR2A | _BV(COM2B1)) & ~(_BV(COM2B0) | _BV(COM2A1) | _BV(COM2A0)) |
           _BV(WGM21) | _BV(WGM20);
  // No prescaler (p.162)
  TCCR2B = (TCCR2B & ~(_BV(CS22) | _BV(CS21))) | _BV(CS20) | _BV(WGM22);
  
  OCR2A = pow(2,COMPARE_BITS)-1;
  OCR2B = 0;
  // Configure the ADC and Timer1 to trigger automatic interrupts
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM13) | _BV(WGM12);
  ICR1 = ((F_CPU / 8) / REFCLK) - 1;
  ADMUX = _BV(REFS0) | _BV(ADLAR) | 0; // Channel 0, shift result left (ADCH used)
  DDRC &= ~_BV(0);
  PORTC &= ~_BV(0);
  DIDR0 |= _BV(0);
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2); // | _BV(ADPS0);  */
}
  
AFSK::PacketBuffer::PacketBuffer() {
  nextPacketIn = 0;
  nextPacketOut = 0;
  inBuffer = 0;
  for(unsigned char i = 0; i < PACKET_BUFFER_SIZE; ++i) {
    packets[i] = 0x0;
  }
#ifdef PACKET_PREALLOCATE
  for(unsigned char i = 0; i < PPOOL_SIZE; ++i) {
    // Put some empty packets in the FIFO
    preallocPool.enqueue(&preallocPackets[i]);
  }
#endif
}

unsigned char AFSK::PacketBuffer::readyCount() volatile {
  unsigned char i;
  unsigned int cnt = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for(i = 0; i < PACKET_BUFFER_SIZE; ++i) {
      if(packets[i] && packets[i]->ready)
        ++cnt;
    }
  }  
  return cnt;
}
  
// Return NULL on empty packet buffers
AFSK::Packet *AFSK::PacketBuffer::getPacket() volatile {
  unsigned char i = 0;
  AFSK::Packet *p = NULL;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if(inBuffer == 0) {
      return 0x0;
    }
    
    do {
      p = packets[nextPacketOut];
      if(p) {
        packets[nextPacketOut] = 0x0;
        --inBuffer;
      }
      nextPacketOut = ++nextPacketOut % PACKET_BUFFER_SIZE;
      ++i;
    } while(!p && i<PACKET_BUFFER_SIZE);
    
    // Return whatever we found, if anything
  }  
  return p;
}
  
//void Packet::init(uint8_t *buf, unsigned int dlen, bool freeData) {
void AFSK::Packet::init(unsigned short dlen) {
  //data = (unsigned char *)buf;
  ready = 0;
#ifdef PACKET_PREALLOCATE
  freeData = 0;
  maxLen = PACKET_MAX_LEN; // Put it here instead
#else
  freeData = 1;
  dataPtr = (uint8_t *)malloc(dlen+16);
  maxLen = dlen; // Put it here instead
#endif
  type = PACKET_STATIC;
  len = 0; // We had a length, but don't put it here.
  dataPos = dataPtr;
  readPos = dataPtr;
  fcs = 0xffff;
}
  
// Allocate a new packet with a data buffer as set
AFSK::Packet *AFSK::PacketBuffer::makePacket(unsigned short dlen) {
  AFSK::Packet *p;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    //Packet *p = findPooledPacket();
#ifdef PACKET_PREALLOCATE
    if(preallocPool.count())
      p = preallocPool.dequeue();
    else p = NULL;
#else
    p = new Packet(); //(Packet *)malloc(sizeof(Packet));
#endif
    if(p) // If allocated
      p->init(dlen);
  }
  return p; // Passes through a null on failure.
}
  
// Free a packet struct, mainly convenience
void AFSK::PacketBuffer::freePacket(Packet *p) {
  if(!p)
    return;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#ifdef PACKET_PREALLOCATE
    preallocPool.enqueue(p);
#else
    p->free();
    /*unsigned char i;
    for(i = 0; i < PPOOL_SIZE; ++i)
    if(p == &(pPool[i]))
    break;
    if(i < PPOOL_SIZE)
    pStatus &= ~(1<<i);*/
    delete p;
#endif
  }  
}
  
// Put a packet onto the buffer array
bool AFSK::PacketBuffer::putPacket(Packet *p) volatile {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if(inBuffer >= PACKET_BUFFER_SIZE) {
      return false;
    }
    packets[nextPacketIn] = p;
    nextPacketIn = ++nextPacketIn % PACKET_BUFFER_SIZE;
    ++inBuffer;
  }  
  return true;
}
  
// Print a single byte to the data array
size_t AFSK::Packet::write(uint8_t c) {
  return (appendFCS(c)?1:0);
}
  
size_t AFSK::Packet::write(const uint8_t *ptr, size_t len) {
  size_t i;
  for(i = 0; i < len; ++i)
    if(!appendFCS(ptr[i]))
      break;
  return i;
}

// Add a callsign, flagged as source, destination, or digi
// Also tell the routine the SSID to use and if this is the final callsign
size_t AFSK::Packet::appendCallsign(const char *callsign, uint8_t ssid, bool final) {
  uint8_t i;
  for(i = 0; i < strlen(callsign) && i < 6; i++) {
    appendFCS(callsign[i]<<1);
  }
  if(i < 6) {
    for(;i<6;i++) {
      appendFCS(' '<<1);
    }
  }
  uint8_t ssidField = (ssid&0xf) << 1;
  // TODO: Handle digis in the address C bit
  if(final) {
    ssidField |= 0b01100001;
  } else {
    ssidField |= 0b11100000;
  }
  appendFCS(ssidField);
}

#ifdef PACKET_PARSER
// Process the AX25 frame and turn it into a bunch of useful strings
bool AFSK::Packet::parsePacket() {
  uint8_t *d = dataPtr;
  int i;
  
  // First 7 bytes are destination-ssid
  for(i = 0; i < 6; i++) {
    dstCallsign[i] = (*d++)>>1;
    if(dstCallsign[i] == ' ') {
      dstCallsign[i] = '\0';
    }
  }
  dstCallsign[6] = '\0';
  dstSSID = ((*d++)>>1) & 0xF;
  
  // Next 7 bytes are source-ssid
  for(i = 0; i < 6; i++) {
    srcCallsign[i] = (*d++)>>1;
    if(srcCallsign[i] == ' ') {
      srcCallsign[i] = '\0';
    }
  }
  srcCallsign[6] = '\0';
  srcSSID = *d++; // Don't shift yet, we need the LSB
  
  digipeater[0][0] = '\0'; // Set null in case we have none anyway
  if((srcSSID & 1) == 0) { // Not the last address field 
    int digi; // Which digi we're on
    for(digi = 0; digi < 8; digi++) {
      for(i = 0; i < 6; i++) {
        digipeater[digi][i] = (*d++)>>1;
        if(digipeater[digi][i] == ' ') {
          digipeater[digi][i] = '\0';
        }
      }
      uint8_t last = (*d) & 1;
      digipeaterSSID[digi] = ((*d++)>>1) & 0xF;
      if(last == 1)
        break;
    }
    digipeater[digi][6] = '\0';
    for(digi += 1; digi<8; digi++) { // Empty out the rest of them
      digipeater[digi][0] = '\0';
    }
  }
  
  // Now handle the SSID itself
  srcSSID >>= 1;
  srcSSID &= 0xF;
  
  // After the address parsing, we end up on the control field
  control = *d++;
  // We have a PID if control type is U or I
  // Control & 1 == 0 == I frame
  // Control & 3 == 3 == U frame
  if((control & 1) == 0 || (control & 3) == 3)
    pid = *d++;
  else pid = 0;
  
  // If there is no PID, we have no data
  if(!pid) {
    iFrameData = NULL;
    return true;
  }
  
  // At this point, we've walked far enough along that data is just at d
  iFrameData = d;
  
  // Cheat a little by setting the first byte of the FCS to 0, making it a string
  // First FCS byte is found at -2, HDLC flags aren't in this buffer
  dataPtr[len-2] = '\0';
  
  return true;
}
#endif

void AFSK::Packet::printPacket(Stream *s) {
  uint8_t i;
#ifdef PACKET_PARSER
  if(!parsePacket()) {
    s->print(F("Packet not valid"));
    return;
  }
  
  s->print(srcCallsign);
  if(srcSSID > 0) {
    s->write('-');
    s->print(srcSSID);
  }
  s->print(F(" > "));
  s->print(dstCallsign);
  if(dstSSID > 0) {
    s->write('-');
    s->print(dstSSID);
  }
  s->write(' ');
  if(digipeater[0][0] != '\0') {
    s->print(F("via "));
    for(i = 0; i < 8; i++) {
      if(digipeater[i][0] == '\0')
        break;
      s->print(digipeater[i]);
      if(digipeaterSSID[i] != 0) {
        s->write('-');
        s->print(digipeaterSSID[i]);
      }
      if((digipeaterSSID[i] & _BV(7)) == _BV(7)) {
        s->write('*'); // Digipeated already
      }
      // If we might have more, check to add a comma
      if(i < 7 && digipeater[i+1][0] != '\0') {
        s->write(',');
      }
      s->write(' ');
    }
  }
  
  // This is an S frame, we can only print control info
  if(control & 3 == 1) {
    switch((control>>2)&3) {
      case 0:
        s->print(F("RR"));
        break;
      case 1:
        s->print(F("RNR"));
        break;
      case 2:
        s->print(F("REJ"));
        break;
      case 3: // Undefined
        s->print(F("unk"));
        break;
    }
    // Use a + to indicate poll bit
    if(control & _BV(4) == _BV(4)) {
      s->write('+');
    }
  } else if((control & 3) == 3) { // U Frame
    s->print(F("U("));
    s->print(control, HEX);
    s->write(',');
    s->print(pid, HEX);
    s->print(F(") "));
  } else if((control & 1) == 0) { // I Frame
    s->print(F("I("));
    s->print(control, HEX);
    s->write(',');
    s->print(pid, HEX);
    s->print(F(") "));
  }
  s->print(F("len "));
  s->print(len);
  s->print(F(": "));
  s->print((char *)iFrameData);
  s->println();
#else // no packet parser, do a rudimentary print
  // Second 6 bytes are source callsign
  for(i=7; i<13; i++) {
    s->write(*(dataPtr+i)>>1);
  }
  // SSID
  s->write('-');
  s->print((*(dataPtr+13) >> 1) & 0xF);
  s->print(F(" -> "));
  // First 6 bytes are destination callsign
  for(i=0; i<6; i++) {
    s->write(*(dataPtr+i)>>1);
  }
  // SSID
  s->write('-');
  s->print((*(dataPtr+6) >> 1) & 0xF);
  // Control/PID next two bytes
  // Skip those, print payload
  for(i = 15; i<len; i++) {
    s->write(*(dataPtr+i));
  }
#endif
}

// Determine what we want to do on this ADC tick.
void AFSK::timer() {
  static uint8_t tcnt = 0;
  if(++tcnt == T_BIT && encoder.isSending()) {
    PORTD |= _BV(6);
    // Only run the encoder every 8th tick
    // This is actually DDS RefClk / 1200 = 8, set as T_BIT
    // A different refclk needs a different value
    encoder.process();
    tcnt = 0;
    PORTD &= ~_BV(6);
  } else {
    decoder.process(((int8_t)(ADCH - 128)));
  }
}

void AFSK::start(DDS *dds) {
  afskEnabled = true;
  encoder.setDDS(dds);
  decoder.start();
}
