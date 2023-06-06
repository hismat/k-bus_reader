/*
MIT License

Copyright (c) 2023 Takashi Hisamatsu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// If there is no new byte from RX buffer for more than IDLE_WAIT_INTERVAL, 
// it is consiered as the k-bus is idle state.
#define IDLE_WAIT_INTERVAL 50 //ms

// defines related to K-Bus protocol
#define KBUS_MAX_DATA_LEN 32
#define KBUS_BYTE_POS_SRC 0
#define KBUS_BYTE_POS_LEN 1
#define KBUS_BYTE_POS_DST 2
#define KBUS_BYTE_POS_DAT 3

// Debug related parameters/flags here
#define DEBUG


// This program is designed based on a small statemachine.
// There are three states that represent the bus state
// unknown: The Arduino cannot tell whether some message is being
//          sent over the k-bus or not.
// idle: No message is currently being sent over the k-bus.
// occupied: Some message is currently being sent over the k-bus.
// invalidState: Just an initial value of variable.
enum kBusState
{
  unknown,
  idle,
  occupied,
  invalidState
};


// The Arduino detects below events to run the statemachine.
// intervalTimeout: No new byte is received from RX buffer consecutively for period of IDLE_WAIT_INTERVAL
// srcByteRecv: The first byte of k-bus message is received.
// xorOk: Whole k-bus message was received and checksum match.
// xorOk: Whole k-bus message was received and checksum unmatch.
// invalidLen: The second byte of k-bus message indicates more than 34 bytes. (32bytes for data and 2bytes for destination ID and checksum)
enum kBusEvent
{
  intervalTimeout,
  srcByteRecv,
  xorOk,
  xorNok,
  invalidLen,
  invalidEvent
};


// Data struct of a k-bus message.
typedef struct 
{
  byte src; // Source ID
  byte len; // Total length of below bytes
  byte dst; // Destination ID
  byte dat[KBUS_MAX_DATA_LEN]; // Payload data
  byte chk; // Checksum of the message
  byte xorVal; // XOR calculation result by Arduino
} kBusMessage;


kBusMessage kBusBuf;
kBusState state = invalidState;
unsigned char bitPos = 0;
unsigned char bytePos = 0;
byte byteBuf = 0;

unsigned char bufReadPos = 0;

void clearKbusBuf()
{
  kBusBuf.src = 0;
  kBusBuf.len = 0;
  kBusBuf.dst = 0;
  kBusBuf.chk = 0;
  kBusBuf.xorVal = 0;
  memset(&kBusBuf, 0, KBUS_MAX_DATA_LEN);
  bitPos = 0;
  bytePos = 0;
  byteBuf = 0;
}

void kBusStatemachine(kBusEvent evt)
{
  kBusState newState = invalidState;
  switch(state)
  {
    case unknown:
      if(evt == intervalTimeout)
      {
        newState = idle;
      }
      break;
    case idle:
      if(evt == srcByteRecv)
      {
        newState = occupied;
      }
      break;
    case occupied:
      if(evt == xorOk)
      {
        newState = idle;
      }
      else if(evt == xorNok)
      {
        newState = unknown;
      }
      else if(evt == invalidLen)
      {
        newState = unknown;
      }
      break;
  }

  //exit acitivity
  if(state != newState && newState != invalidState)
  {
    switch(state)
    {
      case unknown:
        break;
      case idle:
        break;
      case occupied:
        break;
    }
  }

  //transition
  switch(evt)
  {
      case intervalTimeout:
        break;
      case srcByteRecv:
        break;
      case xorOk:
      // Here to add some task to be executed upon a new complete K-Bus message is received.
#ifdef DEBUG
        sendDebugSerial(kBusBuf);
#endif
        break;
      case xorNok:
#ifdef DEBUG
        sendDebugSerial(kBusBuf);
#endif
        break;
      case invalidEvent:
        break;
  }

  //entry activity
  switch(newState)
  {
    case unknown:
      break;
    case idle:
      break;
    case occupied:
      clearKbusBuf();
      kBusBuf.src = Serial1.read();
      bytePos++;
      break;
  }
  state = newState;
}

#ifdef DEBUG

void sendDebugSerial(kBusMessage msg)
{
  char serialTxBuf[512];
  unsigned char i = 0;
  sprintf(serialTxBuf, "src:%02x,len:%u,dst:%02x,dat:",msg.src, msg.len, msg.dst);
  for(i = 0; i < (msg.len - 2); i++)
  {
    if(i == (msg.len - 3))
    {
      sprintf(serialTxBuf, "%s%02x",serialTxBuf, msg.dat[i]);
    }
    else
    {
      sprintf(serialTxBuf, "%s%02x ",serialTxBuf, msg.dat[i]);
    }
  }
  sprintf(serialTxBuf, "%s,chk:%02x,xor:%02x",serialTxBuf, msg.chk, msg.xorVal);
  Serial.println(serialTxBuf);
}

#endif

void setup() {
  // put your setup code here, to run once:
  state = unknown;

#ifdef DEBUG
  Serial.begin(115200);
#endif

  Serial1.begin(9600, SERIAL_8E1);
}

void loop() {
  // put your main code here, to run repeatedly:
  kBusEvent event = invalidEvent;

  //Do activity & Event detection
  switch(state)
  {
    case unknown:
      while(Serial1.available())
      {
        //Clear buffer
        byteBuf = Serial1.read();
      }
      delay(IDLE_WAIT_INTERVAL);
      if(!Serial1.available())
      {
        event = intervalTimeout;
      }
      break;
    case idle:
      if(Serial1.available())
      {
        event = srcByteRecv;
      }
      break;
    case occupied:
      if(Serial1.available())
      {
        switch(bytePos)
        {
          case KBUS_BYTE_POS_LEN:
            kBusBuf.len = Serial1.read();
            kBusBuf.xorVal = kBusBuf.src ^ kBusBuf.len;
            
            //error check on length
            if(kBusBuf.len > KBUS_MAX_DATA_LEN + 2)
            {
              event = invalidLen;
            }
            bytePos ++;
            break;
          case KBUS_BYTE_POS_DST:
            kBusBuf.dst = Serial1.read();
            kBusBuf.xorVal = kBusBuf.xorVal ^ kBusBuf.dst;
            bytePos ++;
            break;
          default:
            //Data read
            if(bytePos >= KBUS_BYTE_POS_DAT && bytePos < (1 + kBusBuf.len))
            {
              kBusBuf.dat[(bytePos - KBUS_BYTE_POS_DAT)] = Serial1.read();
              kBusBuf.xorVal = kBusBuf.xorVal ^ kBusBuf.dat[(bytePos - KBUS_BYTE_POS_DAT)];
              bytePos ++;
            }
            else if(bytePos == (1 + kBusBuf.len))
            {
              kBusBuf.chk = Serial1.read();
              if(kBusBuf.chk == kBusBuf.xorVal)
              {
                event = xorOk;
              }
              else
              {
                event = xorNok;
              }
              bytePos ++;
            }
            break;
        }
      }
      break;
  }
  if(event != invalidEvent)
  {
    kBusStatemachine(event);
  }
}
