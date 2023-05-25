#define IDLE_WAIT_INTERVAL 50 //ms
#define KBUS_MAX_DATA_LEN 12

#define KBUS_BYTE_POS_SRC 0
#define KBUS_BYTE_POS_LEN 1
#define KBUS_BYTE_POS_DST 2
#define KBUS_BYTE_POS_DAT 3

#define LED_RED 3
#define LED_GREEN 4
#define LED_BLUE 5
#define LED_YELLOW 6

#define DEBUG
#define RICH_DEBUG

enum busReadState
{
  unknown,
  idle,
  readingBus,
  invalidState
};

enum kBusEvent
{
  intervalTimeout,
  srcByteRecv,
  xorOk,
  xorNok,
  invalidLen,
  invalidEvent
};

typedef struct 
{
  byte src;
  byte len;
  byte dst;
  byte dat[KBUS_MAX_DATA_LEN];
  byte chk;
  byte xorVal;
} kBusMessage;


kBusMessage kBusBuf;
busReadState state = invalidState;
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
  busReadState newState = invalidState;
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
        newState = readingBus;
      }
      break;
    case readingBus:
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
      case readingBus:
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
    case readingBus:
      clearKbusBuf();
      kBusBuf.src = Serial1.read();
      bytePos++;
      break;
  }
  state = newState;
#ifdef DEBUG
  debugLed(state);
#endif
}

#ifdef DEBUG
void debugLed(busReadState state)
{
  switch(state)
  {
    case unknown:
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE, HIGH);
      digitalWrite(LED_YELLOW, HIGH);
      break;
    case idle:
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE, LOW);
      digitalWrite(LED_YELLOW, LOW);
      break;
    case readingBus:
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      break;
  }
}

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
  debugLed(state);
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
    case readingBus:
      if(Serial1.available())
      {
        switch(bytePos)
        {
          case KBUS_BYTE_POS_LEN:
            kBusBuf.len = Serial1.read();
            kBusBuf.xorVal = kBusBuf.src ^ kBusBuf.len;
            
            //error check on length
            if(kBusBuf.len > KBUS_MAX_DATA_LEN)
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
