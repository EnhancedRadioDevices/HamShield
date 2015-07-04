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

#define T_BIT ((unsigned int)(9600/1200))

#ifdef PACKET_PREALLOCATE
SimpleFIFO<AFSK::Packet *,PPOOL_SIZE> preallocPool;
#endif

void AFSK::Encoder::process() {
  // Check what clock pulse we're on
  if(bitClock == 0) { // We are onto our next bit timing  
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
        }
    
        // We ran out of actual data, provide an HDLC frame (idle)
        if(currentBytePos++ == packet->len) {
          pBuf.freePacket(packet);
          packet = pBuf.getPacket(); // Get the next, if any
          currentBytePos = 0;
          currentByte = HDLC_FRAME;
          hdlc = true;
        } else {
          // Grab the next byte
          currentByte = packet->getByte(); //[currentBytePos++];
          if(currentByte == HDLC_ESCAPE) {
            currentByte = packet->getByte(); //[currentBytePos++];
            hdlc = true;
          } else {
            hdlc = false;
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
  }

  // Advance the bitclock here, to let first bit be sent early 
  if(++bitClock == T_BIT)
    bitClock = 0;

  if(currentTone == 0) {
    dds->setFrequency(AFSK_SPACE);
  } else {
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
  bitClock = 0;
  preamble = 0b110000; // 6.7ms each, 23 = 153ms
  done = false;
  hdlc = true;
  packet = 0x0; // No initial packet, find in the ISR
  currentBytePos = 0;
  maxTx = 3;
  sending = true;
  dds->setFrequency(0);
  dds->on();
  return true;
}

void AFSK::Encoder::stop() {
  randomWait = 0;
  sending = false;
  done = true;
  dds->off();
}

AFSK::Decoder::Decoder() {
  // Initialize the sampler delay line (phase shift)
  for(unsigned char i = 0; i < SAMPLEPERBIT/2; i++)
    delay_fifo.enqueue(0);
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

#define FASTRING_SIZE 4
#define FASTRING_MASK (FASTRING_SIZE-1)
template <typename T, int size>
class FastRing {
private:
  T ring[FASTRING_SIZE];
  uint8_t position;
public:
  FastRing(): position(0) {}
  inline void write(T value) {
    ring[(position++) & FASTRING_MASK] = value;
  }
  inline T read() const {
    return ring[position & FASTRING_MASK];
  }
  inline T readn(uint8_t n) const {
    return ring[(position + (~n+1)) & FASTRING_MASK];
  }
};
FastRing<uint8_t,4> delayLine;

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
      currentPacket->append(HDLC_ESCAPE); // Append without FCS
      c = rx_fifo.dequeue(); // Reset to the next character
    }
    
    // Append all the bytes
    // This will include unescaped HDLC_FRAME bytes
    //if(c == HDLC_FRAME && !escaped)
    //currentPacket->append(c); // Framing bytes don't get FCS updates
    //else
    if(c != HDLC_FRAME)
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
                if(currentPacket->getByte() == 0x03 && currentPacket->getByte() == 0xf0) {
                  filtered = true;
                  break; // Found it
                }
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
  maxLen = 128; // Put it here instead
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
    p = preallocPool.dequeue();
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

// Determine what we want to do on this ADC tick.
void AFSK::timer() {
  if(encoder.isSending())
    encoder.process();
  else
    decoder.process(ADCH - 128);
}

void AFSK::start(DDS *dds) {
  afskEnabled = true;
  encoder.setDDS(dds);
  decoder.start();
}
