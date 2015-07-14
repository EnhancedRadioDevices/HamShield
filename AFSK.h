#ifndef _AFSK_H_
#define _AFSK_H_

#include <Arduino.h>
#include <SimpleFIFO.h>
#include <DDS.h>

#define SAMPLERATE 9600
#define BITRATE    1200

#define SAMPLEPERBIT (SAMPLERATE / BITRATE)

#define RX_FIFO_LEN 16

#define PACKET_BUFFER_SIZE 2
#define PACKET_STATIC 0

// Enable the packet parser which will tokenize the AX25 frame into easy strings
#define PACKET_PARSER

// If this is set, all the packet buffers will be pre-allocated at compile time
// This will use more RAM, but can make it easier to do memory planning.
// TODO: Make this actually work right and not crash.
#define PACKET_PREALLOCATE

// This is with all the digis, two addresses, and full payload
// Dst(7) + Src(7) + Digis(56) + Ctl(1) + PID(1) + Data(0-256) + FCS(2)
#define PACKET_MAX_LEN 330
// Minimum is Dst + Src + Ctl + FCS
#define AX25_PACKET_HEADER_MINLEN 17

// HDLC framing bits
#define HDLC_FRAME    0x7E
#define HDLC_RESET    0x7F
#define HDLC_PREAMBLE 0x00
#define HDLC_ESCAPE   0x1B
#define HDLC_TAIL     0x1C

class AFSK {
private:
  volatile bool afskEnabled;
public:
  bool enabled() { return afskEnabled; };
  
  class Packet:public Print {
  public:
    Packet():Print() {};
    virtual size_t write(uint8_t);
    // Stock virtual method does what we want here.
    //virtual size_t write(const char *);
    virtual size_t write(const uint8_t *, size_t);
    using Print::write;
    unsigned char ready : 1;
    unsigned char type : 2;
    unsigned char freeData : 1;
    unsigned short len;
    unsigned short maxLen;
    //void init(uint8_t *buf, unsigned int dlen, bool freeData); 
    void init(unsigned short dlen);
    inline void free() {
      if(freeData)
        ::free(dataPtr);
    }
    inline const unsigned char getByte(void) {
      return *readPos++;
    }
    inline const unsigned char getByte(uint16_t p) {
      return *(dataPtr+p);
    }
    inline void start() {
      fcs = 0xffff;
      // No longer put an explicit frame start here
      //*dataPos++ = HDLC_ESCAPE;
      //*dataPos++ = HDLC_FRAME;
      //len = 2;
      len = 0;
    }
    
    inline bool append(char c) {
      if(len < maxLen) {
        ++len;
        *dataPos++ = c;
        return true;
      }
      return false;	  
    }
    
    #define UPDATE_FCS(d) e=fcs^(d); f=e^(e<<4); fcs=(fcs>>8)^(f<<8)^(f<<3)^(f>>4)
    //#define UPDATE_FCS(d) s=(d)^(fcs>>8); t=s^(s>>4); fcs=(fcs<<8)^t^(t<<5)^(t<<12)
    inline bool appendFCS(unsigned char c) {
      register unsigned char e, f;
      if(len < maxLen - 4) { // Leave room for FCS/HDLC
        append(c);
        UPDATE_FCS(c);
        return true;
      }
      return false;
    }
    
    size_t appendCallsign(const char *callsign, uint8_t ssid, bool final = false);
    
    inline void finish() {
      append(~(fcs & 0xff));
      append(~((fcs>>8) & 0xff));
      // No longer append the frame boundaries themselves
      //append(HDLC_ESCAPE);
      //append(HDLC_FRAME);
      ready = 1;
    }
    
    inline void clear() {
      fcs = 0xffff;
      len = 0;
      readPos = dataPtr;
      dataPos = dataPtr;
    }
    
    inline bool crcOK() {
      return (fcs == 0xF0B8);
    }
#ifdef PACKET_PARSER
    bool parsePacket();
#endif
    void printPacket(Stream *s);
  private:
#ifdef PACKET_PREALLOCATE
    uint8_t dataPtr[PACKET_MAX_LEN]; // 256 byte I frame + headers max of 78
#else
    uint8_t *dataPtr;
#endif
#ifdef PACKET_PARSER
    char srcCallsign[7];
    uint8_t srcSSID;
    char dstCallsign[7];
    uint8_t dstSSID;
    char digipeater[8][7];
    uint8_t digipeaterSSID[8];
    uint8_t *iFrameData;
    uint8_t length;
    uint8_t control;
    uint8_t pid;
#endif
    uint8_t *dataPos, *readPos;
    unsigned short fcs;
  };
  
  
  class PacketBuffer {
  public:
    // Initialize the buffers
    PacketBuffer();
    // How many packets are in the buffer?
    unsigned char count() volatile { return inBuffer; };
    // And how many of those are ready?
    unsigned char readyCount() volatile;
    // Retrieve the next packet
    Packet *getPacket() volatile;
    // Create a packet structure as needed
    // This does not place it in the queue
    static Packet *makePacket(unsigned short);
    // Conveniently free packet memory
    static void freePacket(Packet *);
    // Place a packet into the buffer
    bool putPacket(Packet *) volatile;
  private:
    volatile unsigned char inBuffer;
    Packet * volatile packets[PACKET_BUFFER_SIZE];
    volatile unsigned char nextPacketIn;
    volatile unsigned char nextPacketOut;
  };
  
  class Encoder {
  public:
    Encoder() {
      randomWait = 1000; // At the very begin, wait at least one second
      sending = false;
      done = true;
      packet = 0x0;
      currentBytePos = 0;
      nextByte = 0;
    }
    void setDDS(DDS *d) { dds = d; }
    volatile inline bool isSending() volatile { 
      return sending; 
    }
    volatile inline bool isDone() volatile { 
      return done; 
    }
    volatile inline bool hasPackets() volatile { 
      return (pBuf.count() > 0); 
    }
    inline bool putPacket(Packet *packet) {
      return pBuf.putPacket(packet);
    }
    inline void setRandomWait() {
      randomWait = 250 + (rand() % 1000) + millis();
    }
    bool start();
    void stop();
    void process();
  private:
    volatile bool sending;
    byte currentByte;
    byte currentBit : 1;
    byte currentTone : 1;
    byte lastZero : 3;
    byte bitPosition : 3;
    byte preamble : 6;
    //byte bitClock;
    bool hdlc;
    byte nextByte;
    byte maxTx;
    Packet *packet;
    PacketBuffer pBuf;
    unsigned int currentBytePos;
    volatile unsigned long randomWait;
    volatile bool done;
    DDS *dds;
  };
  
  class HDLCDecode {
  public:
    bool hdlcParse(bool, SimpleFIFO<uint8_t,RX_FIFO_LEN> *fifo);
    volatile bool rxstart;
  private:
    uint8_t demod_bits;
    uint8_t bit_idx;
    uint8_t currchar;
  };
  
  class Decoder {
  public:
    Decoder();
    void start();
    bool read();
    void process(int8_t);
    inline bool dataAvailable() {
      return (rx_fifo.count() > 0);
    }
    inline uint8_t getByte() {
      return rx_fifo.dequeue();
    }
    inline uint8_t packetCount() volatile {
      return pBuf.count();
    }
    inline Packet *getPacket() {
      return pBuf.getPacket();
    }
    inline bool isReceiving() volatile {
      return hdlc.rxstart;
    }
  private:
    Packet *currentPacket;
    //SimpleFIFO<int8_t,SAMPLEPERBIT/2+1> delay_fifo;
    SimpleFIFO<uint8_t,RX_FIFO_LEN> rx_fifo; // This should be drained fairly often
    int16_t iir_x[2];
    int16_t iir_y[2];
    uint8_t sampled_bits;
    int8_t curr_phase;
    uint8_t found_bits;
    PacketBuffer pBuf;
    HDLCDecode hdlc;
  };
  
public:
  inline bool read() {
    return decoder.read();
  }
  volatile inline bool txReady() volatile {
    if(encoder.isDone() && encoder.hasPackets())
      return true;
    return false;
  }
  volatile inline bool isDone() volatile { return encoder.isDone(); }
  inline bool txStart() {
    if(decoder.isReceiving()) {
      encoder.setRandomWait();
      return false;
    } 
    return encoder.start();
  }
  inline bool putTXPacket(Packet *packet) {
    bool ret = encoder.putPacket(packet);
    if(!ret) // No room?
      PacketBuffer::freePacket(packet);
    return ret;
  }
  inline Packet *getRXPacket() {
    return decoder.getPacket();
  }
  inline uint8_t rxPacketCount() volatile {
    return decoder.packetCount();
  }
  //unsigned long lastTx;
  //unsigned long lastRx;
  void start(DDS *);
  void timer();
  Encoder encoder;
  Decoder decoder;
};
#endif /* _AFSK_H_ */
