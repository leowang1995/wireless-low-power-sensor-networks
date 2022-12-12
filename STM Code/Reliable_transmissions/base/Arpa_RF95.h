/*
  Arpa_RF95.h - Library that implements the protocol specified
  in the ARPA-E Wireless Sensor network specification.

  Authors:
  Bryan Hatasaka
  Merek Goodrich
*/
#ifndef Arpa_RF95_h
#define Arpa_RF95_h
#include <RHReliableDatagram.h>
#include <RH_RF95.h>

#define ARPA_BASE_ID 0

#define ARPA_RECV_TIMEOUT 20000
#define ARPA_TRAN_TIMEOUT 2000
#define ARPA_NUM_RETRIES 3
#define ARPA_FAIL_DELAY 15000
#define APRA_CONNECTION_TIMEOUT 30000
#define APRA_FAIL_DELAYS_MAX 3

// Account for id byte and address byte
#define ARPA_HEADER_LENGTH 2
// 249 bytes
#define ARPA_MAX_MSG_LENGTH RH_RF95_MAX_MESSAGE_LEN - ARPA_HEADER_LENGTH

#define ARPA_ID_BYTE_POS 0
#define ARPA_ADDR_BYTE_POS 1

enum Arpa_msg_type : uint8_t
{
  ARPA_TYPE_ID_INVALID = 0x0,
  ARPA_TYPE_ID_SYN = 0x1,
  ARPA_TYPE_ID_FIN = 0x2,
  ARPA_TYPE_ID_ACK = 0x3,
  ARPA_TYPE_ID_NACK = 0x4,
  ARPA_TYPE_ID_CHECK = 0x5,
  ARPA_TYPE_ID_DATA = 0xA,
  ARPA_TYPE_ID_TIME = 0xB
};

class Arpa_RF95
{
public:
  uint8_t baseId = ARPA_BASE_ID;

  Arpa_RF95(RH_RF95 *driver, uint8_t _rst, float _freq, int8_t _power, uint8_t _en, uint8_t _nodeId);

  /// Initializes the module with the correct settings and prepares it to receive and send
  /// data.
  /// Must be called before sending/reciving data for the first time.
  /// Must be called after waking the module from sleep.
  ///
  /// \return True if the module was initilized correctly, else false.
  bool InitModule();

  /// Sends data through the module with no additional information or formatting.
  /// Must call "SetSendToId()" first to set the reciving node id.
  /// Assumes the data is NULL terminated.
  /// MAXIMUM RH_RF95_MAX_MESSAGE_LEN characters (251 bytes).
  ///
  /// If the module is asleep, it will be awoken and reinitialized before sending
  ///
  /// \param[in] char* data - The null terminated character array to send over LoRa
  /// \param[in] uint8_t sendToId - the id of the LoRa module that data will be
  ///    sent to. Ranges from 1-256
  /// \return bool - true if the data was successfully sent, else false
  bool SendMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data);

  bool SendMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data, const uint8_t len);

  /// Use to send a message from a node to the base after a connection has been made. 
  /// The main difference between this function and SendMessage() is that this function 
  /// will wait for a reply from the base station.
  ///
  /// If the module is asleep, it will be awoken and reinitialized before sending
  ///
  /// \return Arpa_msg_type - return an ack if the message went through well, 
  ///   a nack if the base is not connected to the sender node,
  ///   or invalid if the message could not be sent
  Arpa_msg_type SendConnectedMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data);

  Arpa_msg_type SendConnectedMessage(const uint8_t sendToId, const Arpa_msg_type type, const char *data, const uint8_t len);

  /// Sends data through the module with no additional information or formatting.
  /// Must call "SetSendToId()" first to set the reciving node id.
  /// MAXIMUM RH_RF95_MAX_MESSAGE_LEN characters (251 byte).
  ///
  /// If the module is asleep, it will be awoken and reinitialized before sending
  ///
  /// \param[in] char* data - the character array to send over LoRa
  /// \param[in, out] uint8_t len - the number of characters to send from the array (max 251 bytes)
  /// \return bool - true if the data was successfully sent, else false
  bool SendDatagram(uint8_t sendToId, const uint8_t *data, const uint8_t len);

  void SetReceiveTimeout(const uint16_t timeout);
  void ResetReceiveTimeout();

  void SetTransmitTimeout(const uint16_t timeout);
  void ResetTransmitTimeout();

  /// Block until data is available or until the timeout is reached.
  /// When data is available, the message is copied into buf and true is returned.
  /// Certain message type may have no data sent with them (such as a syn). In this
  /// case, no data will be copied to the input buffer
  /// The timeout is specified by the ARPA_RECV_TIMEOUT macro.
  ///
  /// \param[in, out] uint8_t* buf - the buffer to store the message in
  /// \param[in, out] uint8_t* len - size of the buffer, set to the length of the message after it is copied
  /// \return Arpa_msg_type* type - set to the type of message - invalid if no valid message was copied into buf
  Arpa_msg_type WaitForMessage(uint8_t *buf, uint8_t *len);

  ///
  /// \return Arpa_msg_type* type - set to the type of message if a valid message was copied into buf, 
  ///     invalid if there was an error, and ARPA_TYPE_ID_FIN if the connection was closed
  Arpa_msg_type WaitForConnectedMessage(uint8_t *buf, uint8_t *len);

  /// Sets up this node to constantly wait and forward messages received from nodes to the base. 
  /// Also forwards nodes from the base back to the node based on the message origin id.
  /// Only 1 intermediate node is supported.
  void HandleMessageForwarding();

  /// Delays according to the Failure To Send portion of the protocol.
  /// Internally stores the window size until it is reset.
  bool FailureToSendDelay();
  void ResetFailureToSendDelay();

  /// Set the node base Id. 
  /// This is used for a node if it needs to send to an intermediate 
  /// forwarding node.
  void SetBaseId(const uint8_t id);

  uint8_t GetBaseId() const;

  /// Set the sleep state of the module. True will sleep it, false will wake it up.
  /// 
  /// This function completely disables the module rather than using the 
  /// RFM96W sleep. This means the module is reinitialized after waking and will take
  /// multiple seconds to wake and be ready to send data.
  /// All send methods will wake the module before sending data
  ///
  /// Returns false if the module could not be woken (initialized)
  bool SetSleepState(const bool state);

  const uint8_t GetFromId() const;
  const int16_t GetCurrentConnectionOriginId() const;
  const bool GetSleepState() const;

  // Protocol Implementations

  /// Use to initiate a transaction with another node.
  /// Must be called before sending data to another node to ensure that messages will be
  /// read and recieved correctly by the other node.
  ///
  /// \return bool - true if the sync was successful, else false.
  bool Synchronize();

  /// Used to end a transaction with another node.
  /// Must be called when data is done being sent to another node.
  ///
  /// \return bool - false if the close failed.
  bool Close();

  const int8_t WaitForSyn();

private:
  /// The underlying rf95 object from the RadioHead library
  RHReliableDatagram manager;
  RH_RF95 *driver;

  bool sleepState;
  // signed 16 bit ints for Ids even though tye go from 0-255,
  // -1 is returned for invalid connections
  int16_t currentConnectionId;
  int16_t currentConnectionOriginId;
  
  // originId is the original node the message was sent from
  // fromId is the last node the message was sent from and only used to send back messages
  uint8_t rst, en, power, nodeId, fromId, originId;
  uint16_t numFailedDelays;
  uint32_t failureDelay, timeSinceConnectionActivity;
  float freq;
  uint16_t recvTimeout, tranTimeout;


  uint8_t lastReceivedDatagram[RH_RF95_MAX_MESSAGE_LEN];
  uint16_t lastReceivedDatagramLen;


  // const uint8_t ID_SYN = 0x1;
  // const uint8_t ID_FIN = 0x2;
  // const uint8_t ID_NACK = 0x3;
  // const uint8_t ID_CHECK = 0x4;
  // const uint8_t ID_DATA = 0xA;
  // const uint8_t ID_TIME = 0xB;
};

#endif
