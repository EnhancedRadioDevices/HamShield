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

#define ACCUMULATOR_BITS 24 // This is 2^10 bits used from accum
//#undef PROGMEM
//#define PROGMEM __attribute__((section(".progmem.data")))
const uint8_t PROGMEM sinetable[256] = {
  128,131,134,137,140,143,146,149,152,156,159,162,165,168,171,174,
  176,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216,
  218,220,222,224,226,228,230,232,234,236,237,239,240,242,243,245,
  246,247,248,249,250,251,252,252,253,254,254,255,255,255,255,255,
  255,255,255,255,255,255,254,254,253,252,252,251,250,249,248,247,
  246,245,243,242,240,239,237,236,234,232,230,228,226,224,222,220,
  218,216,213,211,209,206,204,201,199,196,193,191,188,185,182,179,
  176,174,171,168,165,162,159,156,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,99, 96, 93, 90, 87, 84, 81, 
  79, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39, 
  37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10, 
  9,  8,  7,  6,  5,  4,  3,  3,  2,  1,  1,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  1,  1,  2,  3,  3,  4,  5,  6,  7,  8,  
  9,  10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35, 
  37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76, 
  79, 81, 84, 87, 90, 93, 96, 99, 103,106,109,112,115,118,121,124
};

#define AFSK_SPACE 0
#define AFSK_MARK  1

// Timers
volatile unsigned long lastTx = 0;
volatile unsigned long lastTxEnd = 0;
volatile unsigned long lastRx = 0;

#define REFCLK 9600
//#define REFCLK 31372.54902
//#define REFCLK (16000000.0/510.0)
//#define REFCLK 31200.0
// 2200Hz = pow(2,32)*2200.0/refclk
// 1200Hz = pow(2,32)*1200.0/refclk
static const unsigned long toneStep[2] = {
  pow(2,32)*2200.0/REFCLK,
  pow(2,32)*1200.0/REFCLK
};

// Set to an arbitrary frequency
void AFSK::Encoder::setFreq(unsigned long freq, byte vol) {
  unsigned long newStep = pow(2,32)*freq/REFCLK;
  rStep = newStep; // Atomic? (ish)
}

// This allows a programmatic way to tune the output tones
static const byte toneVolume[2] = {
  255,
  255
};

#define T_BIT ((unsigned int)(REFCLK/1200))

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

  accumulator += toneStep[currentTone];
  uint8_t phAng = (accumulator >> ACCUMULATOR_BITS);
  /*if(toneVolume[currentTone] != 255) {
  OCR2B = pwm * toneVolume[currentTone] / 255;
  } else {*/
  // No volume scaling required
  OCR2B = pgm_read_byte_near(sinetable + phAng);
  /*}*/
}

bool AFSK::Encoder::start() {
  if(!done || sending) {
    return false;
  }
  
  if(randomWait > millis()) {
    return false;
  }
  
  accumulator = 0;
  // First real byte is a frame
  currentBit = 0;
  lastZero = 0;
  bitPosition = 0;
  bitClock = 0;
  preamble = 23; // 6.7ms each, 23 = 153ms
  done = false;
  hdlc = true;
  packet = 0x0; // No initial packet, find in the ISR
  currentBytePos = 0;
  maxTx = 3;
  sending = true;
  return true;
}

void AFSK::Encoder::stop() {
  randomWait = 0;
  sending = false;
  done = true;
  OCR2B = 0;
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
  
// Handle the A/D converter interrupt (hopefully quickly :)
void AFSK::Decoder::process(int8_t curr_sample) {
  // Run the same through the phase multiplier and butterworth filter
  iir_x[0] = iir_x[1];
  iir_x[1] = ((int8_t)delay_fifo.dequeue() * curr_sample) >> 2;
  iir_y[0] = iir_y[1];
  iir_y[1] = iir_x[0] + iir_x[1] + (iir_y[0] >> 1) + (iir_y[0]>>3) + (iir_y[0]>>5);
  
  // Shift the bit into place based on the output of the discriminator
  sampled_bits <<= 1;
  sampled_bits |= (iir_y[1] > 0) ? 1 : 0;
  
  // Place this ADC sample into the delay line
  delay_fifo.enqueue(curr_sample);
  
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
  // Configure the ADC and Timer1 to trigger automatic interrupts
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM13) | _BV(WGM12);
  ICR1 = ((F_CPU / 8) / REFCLK) - 1;
  ADMUX = _BV(REFS0) | _BV(ADLAR) | 0; // Channel 0, shift result left (ADCH used)
  DDRC &= ~_BV(0);
  PORTC &= ~_BV(0);
  DIDR0 |= _BV(0);
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2); // | _BV(ADPS0);  
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
  freeData = 1; //freeData;
  type = PACKET_STATIC;
  len = 0; // We had a length, but don't put it here.
  maxLen = dlen; // Put it here instead
  dataPtr = (uint8_t *)malloc(dlen+16);
  dataPos = dataPtr;
  readPos = dataPtr;
  fcs = 0xffff;
}
  
// Allocate a new packet with a data buffer as set
AFSK::Packet *AFSK::PacketBuffer::makePacket(unsigned short dlen) {
  AFSK::Packet *p;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    //Packet *p = findPooledPacket();
    p = new Packet(); //(Packet *)malloc(sizeof(Packet));
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
    p->free();
    /*unsigned char i;
    for(i = 0; i < PPOOL_SIZE; ++i)
    if(p == &(pPool[i]))
    break;
    if(i < PPOOL_SIZE)
    pStatus &= ~(1<<i);*/
    delete p;
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
  
// Determine what we want to do on this ADC tick.
void AFSK::timer() {
  if(encoder.isSending())
    encoder.process();
  decoder.process(ADCH - 128);
}

void AFSK::start() {
  afskEnabled = true;
  decoder.start();
}
