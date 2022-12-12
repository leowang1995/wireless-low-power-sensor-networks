#include "Arpa_RF95.h"
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#ifndef DEBUG
#define DEBUG false
#endif

#if DEBUG == true
#define LOG(msg) (Serial.print(msg))
#define LOG_LN(msg) (Serial.println(msg))
#define LOG_F(msg) (Serial.print(F(msg)))
#define LOG_LN_F(msg) (Serial.println(F(msg)))
#else
#define LOG(msg)
#define LOG_LN(msg)
#define LOG_F(msg)
#define LOG_LN_F(msg)
#endif

Arpa_RF95::Arpa_RF95(RH_RF95 *_driver, uint8_t _rst, float _freq, int8_t _power, uint8_t _en, uint8_t _nodeId)
    : manager(*_driver, _nodeId)
{
  // Set member variables
  this->driver = _driver;
  this->rst = _rst;
  this->freq = _freq;
  this->power = _power;
  this->en = _en;
  this->nodeId = _nodeId;
  this->fromId = 0;
  this->originId = 0;
  this->recvTimeout = ARPA_RECV_TIMEOUT;
  this->failureDelay = ARPA_FAIL_DELAY;
  this->currentConnectionId = -1; // -1 for no connection
  this->currentConnectionOriginId = -1; // -1 for no connection
  this->sleepState = false;
  this->lastReceivedDatagramLen = 0;
  this->numFailedDelays = 0;
  
  // Seed with current node id
  randomSeed(this->nodeId);
  // Set pinmodes
  pinMode(rst, OUTPUT);
  pinMode(en, OUTPUT);

  digitalWrite(this->en, HIGH);

#if DEBUG == true
  if (!Serial)
    Serial.begin(9600);
#endif
}

bool Arpa_RF95::InitModule()
{

  LOG_LN_F("Starting LoRa");

  // Manually reset module (required for Teensy)
  LOG_LN_F("Resetting module");
  digitalWrite(this->rst, LOW);
  delay(10);
  digitalWrite(this->rst, HIGH);
  delay(10);

  // Serial.println("Initializing module");
  while (!this->manager.init())
  {
    LOG_LN_F("LoRa radio initialization failed");
    return false;
  }
  LOG_LN_F("LoRa initialization OK");
  if (!this->driver->setFrequency(this->freq))
  {
    LOG_LN_F("Setting frequency FAILED!");
    return false;
  }

  LOG("Set Freq. to: ");
  LOG_LN(this->freq);

  //comment on March 1st.
  this->driver->setSpreadingFactor(12);
  this->driver->setCodingRate4(8);


  this->driver->setTxPower(this->power, false);
  LOG_F("Set Tx power to: ");
  LOG_LN(this->power);

  this->manager.setTimeout(ARPA_TRAN_TIMEOUT);
  this->manager.setRetries(ARPA_NUM_RETRIES);

  return true;
}

void Arpa_RF95::SetReceiveTimeout(const uint16_t timeout)
{
  this->recvTimeout = timeout;
}

void Arpa_RF95::ResetReceiveTimeout()
{
  this->recvTimeout = ARPA_RECV_TIMEOUT;
}

void Arpa_RF95::SetTransmitTimeout(const uint16_t timeout)
{
  this->tranTimeout = timeout;
}

void Arpa_RF95::ResetTransmitTimeout()
{
  this->tranTimeout = ARPA_RECV_TIMEOUT;
}


bool Arpa_RF95::SendMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data)
{
  LOG_LN_F("Arpa_RF95::SendMessage(const uint8_t, const Arpa_msg_type, const char *)");
  return SendMessage(sendToId, type, data, strlen(data));
}

bool Arpa_RF95::SendMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data, const uint8_t len)
{
  LOG_LN_F("Arpa_RF95::SendMessage(const uint8_t, const Arpa_msg_type, const char *, const uint8_t)");

  if (len > ARPA_MAX_MSG_LENGTH)
  {
    LOG_LN_F("Arpa_RF95::SendMessage(const uint8_t, const Arpa_msg_type, const char *, const uint8_t) failed: len to large");
    return false;
  }

  // Use the full length member buffer, place id byte at beginning and copy the data to the rest of the buffer
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  buf[ARPA_ID_BYTE_POS] = (uint8_t)type;

  // For the base, always put the originId from the node that sent it
  // rather than the base id
  if(this->nodeId == this->baseId)
    buf[ARPA_ADDR_BYTE_POS] = this->originId;
  else
    buf[ARPA_ADDR_BYTE_POS] = this->nodeId;

  memcpy(buf + ARPA_HEADER_LENGTH, data, len);

  // LOG_LN(buf[ARPA_ID_BYTE_POS]);
  // LOG_LN(buf[ARPA_ADDR_BYTE_POS]);
  // LOG_LN(sendToId);

  return this->SendDatagram(sendToId, buf, len + ARPA_HEADER_LENGTH);
}

Arpa_msg_type Arpa_RF95::SendConnectedMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data)
{
  return SendConnectedMessage(sendToId, type, data, strlen(data));
}

Arpa_msg_type Arpa_RF95::SendConnectedMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data, const uint8_t len)
{
  uint8_t receiveBuf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t receiveLen = RH_RF95_MAX_MESSAGE_LEN;
  // Send data
  if(SendMessage(sendToId, type, data, len))
  {
    // Success sending, wait for response and return it
    return WaitForMessage(receiveBuf, &receiveLen);
  }
  return ARPA_TYPE_ID_INVALID;
}

bool Arpa_RF95::SendDatagram(uint8_t sendToId, const uint8_t *data, const uint8_t len)
{
  LOG_LN_F("Arpa_RF95: SendDatagram(uint8_t, const char *, const uint8_t) Sending datagram");

  // Check if module is awake
  if (this->sleepState) // Sleep state true mean the module is asleep
    this->SetSleepState(false);

  if (this->manager.sendtoWait((uint8_t *)data, len, sendToId))
    return true;

  LOG_LN_F("Arpa_RF95: SendDatagram(uint8_t, const char *, const uint8_t) Sending datagram failed");
  return false;
}

Arpa_msg_type Arpa_RF95::WaitForMessage(uint8_t *buf, uint8_t *len)
{
  // Reset our buffer
  memset(this->lastReceivedDatagram, '\0', RH_RF95_MAX_MESSAGE_LEN);

  LOG_LN_F("Arpa_RF95::WaitForMessage(uint8_t *, uint8_t *) Waiting for data");
  if (this->manager.recvfromAckTimeout(this->lastReceivedDatagram, len, this->recvTimeout, &(this->fromId)))
  {
    LOG_LN_F("Arpa_RF95: WaitForMessage(uint8_t *, uint8_t *) Received valid data from module");
    LOG_LN_F("\tfromId:len:message");
    LOG_F("\t");
    LOG(this->fromId);
    LOG_F(":");
    LOG(*len);
    LOG_F(":");
    #if DEBUG == true
    if(*len < RH_RF95_MAX_MESSAGE_LEN)
      this->lastReceivedDatagram[*len] = '\0';          // Null terminate for printing
    #endif
    LOG_LN((char *)(this->lastReceivedDatagram + ARPA_HEADER_LENGTH)); // Remove id and addr byte

    // Get the message type out of the id byte
    Arpa_msg_type msgType = (Arpa_msg_type)this->lastReceivedDatagram[ARPA_ID_BYTE_POS];
    // Get the address out of the header
    this->originId = (uint8_t)this->lastReceivedDatagram[ARPA_ADDR_BYTE_POS];

    LOG_F("\tTypeId: ");
    LOG_LN(msgType);

    LOG_F("\tOriginId: ");
    LOG_LN(this->originId);

    this->lastReceivedDatagramLen = *len;

    // Adjust length to account for id byte and original node address
    *len = *len - ARPA_HEADER_LENGTH;
    // Copy the received data (wihtout id byte and address) to the provided buffer
    memcpy(buf, this->lastReceivedDatagram + ARPA_HEADER_LENGTH, *len);

    return msgType;
  }

  LOG_LN_F("Arpa_RF95::WaitForMessage(uint8_t *, uint8_t *) Timed out or couldn't receive data");
  return ARPA_TYPE_ID_INVALID;
}

Arpa_msg_type Arpa_RF95::WaitForConnectedMessage(uint8_t *buf, uint8_t *len)
{

  if (this->currentConnectionId < 0)
  {
    LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) No current connection");
    LOG_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Current connection id: ");
    LOG_LN(this->currentConnectionId);
    return ARPA_TYPE_ID_INVALID;
  }

  uint8_t bufLen = *len;
  Arpa_msg_type msgType;

  unsigned long currentTime;
  LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Waiting for data");
  // Calculating if it's been more than the timeout period since the last activity from the connected node
  while ((currentTime = millis()) - this->timeSinceConnectionActivity < APRA_CONNECTION_TIMEOUT 
        && currentTime >= this->timeSinceConnectionActivity) // Check that millis hasn't overflowed
  {
    *len = bufLen;
    msgType = this->WaitForMessage(buf, len);
    if (msgType == ARPA_TYPE_ID_INVALID)
    {
      LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Wait for message returned invalid");
      continue;
    }

    // send nack if we get a message from a node not currently connected
    if (this->originId != this->currentConnectionOriginId)
    {
      LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Received message from a not connected node");
      // Send back nack
      this->SendMessage(this->fromId, ARPA_TYPE_ID_NACK, "", 0);

      // Reset current originId
      this->originId = this->currentConnectionOriginId;
      continue;
    }

    switch (msgType)
    {
    case ARPA_TYPE_ID_FIN:
      LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Received fin, closing connection");
      currentConnectionId = -1;
      currentConnectionOriginId = -1;
      return msgType;
      break;
    case ARPA_TYPE_ID_SYN:
      // If a currently connected node tries to send us a syn, then send it back a syn
      // TODO
      break;

    default: // Return any other type of message back to the caller
      // Reply with an ack
      if (!this->SendMessage(this->currentConnectionId, ARPA_TYPE_ID_ACK, "", 0))
      {
        // If we cannot send our ack back, close the connection
        currentConnectionId = -1;
        currentConnectionOriginId = -1;
        return ARPA_TYPE_ID_FIN;
      }

      // Reset timer and return with the data in buf
      this->timeSinceConnectionActivity = millis();
      return msgType;
      break;
    }
  }

  // Timed out
  LOG_LN_F("Arpa_RF95::WaitForConnectedMessage(uint8_t *, uint8_t *) Time out");
  this->currentConnectionId = -1;
  this->currentConnectionOriginId = -1;
  return ARPA_TYPE_ID_FIN;
}

void Arpa_RF95::HandleMessageForwarding()
{
  uint8_t buf[ARPA_MAX_MSG_LENGTH];
  uint8_t len = ARPA_MAX_MSG_LENGTH;
  uint16_t sendId = this->baseId;

  while(true)
  {
    switch(WaitForMessage(buf, &len))
    {
      case ARPA_TYPE_ID_INVALID:
        break;

      default: // If we got a message
        // Check if the message is from the base
        if(this->fromId == this->baseId) // Message base => node
          sendId = this->originId;
        else // Message from node => base
          sendId = this->baseId;
        
        // send entire message to base without modification
        if(!SendDatagram(sendId, this->lastReceivedDatagram, this->lastReceivedDatagramLen))
        {
          // TODO If the message couldn't be sent to the base, maybe tell the node somehow?
        }
        break;
    }

    // Have to reset this as it is used as an input for WaitForMessage
    len = ARPA_MAX_MSG_LENGTH;
  }
}


bool Arpa_RF95::SetSleepState(const bool state)
{
  // Put device to sleep by simply pulling the enable pin low
  if(state)
  {
    digitalWrite(this->en, LOW);
    this->driver->setMode(driver->RHModeSleep);
  }
  // Wake up and re-init
  else
  {
    digitalWrite(this->en, HIGH);
    // Delays to make sure the module wakes up
    delay(50);
    if(!this->InitModule())
      return false;
    
    delay(50); 
  }
  this->sleepState = state;

  return true;
}

bool Arpa_RF95::GetSleepState() const
{
  return this->sleepState;
}

uint8_t Arpa_RF95::GetFromId() const
{
  return this->fromId;
}

int16_t Arpa_RF95::GetCurrentConnectionOriginId() const
{
  return this->currentConnectionOriginId;
}

void Arpa_RF95::SetBaseId(const uint8_t id)
{
  this->baseId = id;
}

void Arpa_RF95::SetNodeId(const uint8_t id)
{
  this->nodeId = id;
}

uint8_t Arpa_RF95::GetBaseId() const
{
  return this->baseId;
}

// Protocol implementations
bool Arpa_RF95::Synchronize()
{
  LOG_LN_F("Arpa_RF95: Synchronize() Building message, sending syn...");
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = RH_RF95_MAX_MESSAGE_LEN;

  // Send a synchronize message
  if (!this->SendMessage(this->baseId, ARPA_TYPE_ID_SYN, "", 0))
  {
    LOG_LN_F("Arpa_RF95: Synchronize() Syn send failed");
    // TODO: handle resending syn according to protocol somehome (either in here or in the caller)
    return false;
  }

  // Wait for a syn back
  switch(this->WaitForMessage(buf, &len))
  {
    case ARPA_TYPE_ID_SYN:
      return true;
      break;

    case ARPA_TYPE_ID_NACK:
      LOG_LN_F("Arpa_RF95: Synchronize() Got a NACK back");
      // TODO: handle resending syn according to protocol somehome (either in here or in the caller)
      return false;
      break;

    case ARPA_TYPE_ID_INVALID:
    default:
      LOG_LN_F("Arpa_RF95: Synchronize() No syn received back");
      return false;
      break;
  }
}

bool Arpa_RF95::Close()
{
  LOG_LN_F("Arpa_RF95: Close() Sending fin...");

  // Send a close message
  if (!this->SendMessage(this->baseId, ARPA_TYPE_ID_FIN, "", 0))
  {
    LOG_LN_F("Arpa_RF95: Synchronize() Fin send failed");
    return false;
  }

  return true;
}

int8_t Arpa_RF95::WaitForSyn()
{
  LOG_LN_F("Arpa_RF95: WaitForSyn() waiting for syn...");

  Arpa_msg_type msgType;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  this->currentConnectionId = -1;
  this->currentConnectionOriginId = -1;

  // Wait forever for a syn message
  while (true)
  {
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN; // The len variable is used as an input and must be set every loop

    msgType = this->WaitForMessage(buf, &len);
    if (msgType == ARPA_TYPE_ID_INVALID)
      continue;

    // Send back syn if we received a syn
    LOG_F("Got a message, type:");
    LOG_LN(msgType);
    if (msgType == ARPA_TYPE_ID_SYN)
    {
      LOG_LN_F("Arpa_RF95: WaitForSyn() Got a syn, sending one back");

      if (this->SendMessage(this->fromId, ARPA_TYPE_ID_SYN, "", 0))
      {
        this->currentConnectionId = this->fromId;
        this->currentConnectionOriginId = this->originId;
        this->timeSinceConnectionActivity = millis();
        break;
      }
    }
    else if (msgType == ARPA_TYPE_ID_CHECK)
    {
      this->SendMessage(this->fromId, ARPA_TYPE_ID_CHECK, "", 0);
    }
    else
    {
      // Send back nack if a node tries to send something other than a syn or check
      this->SendMessage(this->fromId, ARPA_TYPE_ID_NACK, "", 0);
      LOG_LN_F("Arpa_RF95: WaitForSyn() Timed out or got no syn");
    }
  }

  return currentConnectionOriginId;
}

bool Arpa_RF95::FailureToSendDelay()
{
  if (this->numFailedDelays > APRA_FAIL_DELAYS_MAX)
    return false;
  
  // Delay according to protocol (random val between the faiureDelay and failureDelay-5)
  delay(random(this->failureDelay - 5000, this->failureDelay) * 1L);
  this->failureDelay *= 2;

  // Limit the failure delay to 30 seconds between sends
  if(this->failureDelay > 30000)
    this->failureDelay = 30000;
  
  ++this->numFailedDelays;
  return true;
}

void Arpa_RF95::ResetFailureToSendDelay()
{
  this->failureDelay = ARPA_FAIL_DELAY;
}
