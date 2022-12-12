#pragma once
#include <Arduino.h>

class Configuration
{
public:
enum NodeType
{
  sensor = 0,
  forwarder = 1,
  base = 2,
  invalid = -1
};

Configuration();
void StartConfiguration(int sigPin);
uint8_t GetEEPromNodeId();
uint8_t GetEEPromBaseId();
NodeType GetEEPromNodeType();

private:
void ReadSerial();

// Commands
void cmdGetNodeId();
void cmdGetBaseId();
void cmdGetType();

void cmdSetNodeId();
void cmdSetBaseId();
void cmdSetType();

// Parsing helpers
char *ParseTok(char*);
int16_t ParseNumber(char*);
int16_t ParseNumArg();
void SendSerialNum(int);

// EEPROM Helpers
void SetEEPromNodeId(uint8_t);
void SetEEPromBaseId(uint8_t);
void SetEEPromNodeType(NodeType);


// Variables
static const int bufSize = 32;
static const int EEPROM_NodeIdOffset = 1;
static const int EEPROM_BaseIdOffset = 2;
static const int EEPROM_NodeTypeOffset = 3;
static const int baud = 9600;
char serialCommandBuffer[bufSize];
struct Node
{
  uint8_t nodeId;
  uint8_t baseId;
  NodeType type;
} currentNode;

};