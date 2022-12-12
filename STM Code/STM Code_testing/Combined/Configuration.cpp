#include "Configuration.h"
#include <string.h>
#include <EEPROM.h>

Configuration::Configuration()
{
    this->currentNode.nodeId = GetEEPromNodeId();
    this->currentNode.baseId = GetEEPromBaseId();
    this->currentNode.type = GetEEPromNodeType();
}

void Configuration::StartConfiguration(int sigPin)
{
    // Wait until the signal pin drops low
    // then start Serial and delay 50 ms before sending a CRLF
    while(digitalRead(sigPin) == HIGH) {}
    Serial.begin(this->baud);
    delay(50);
    Serial.print("Node Configuration\r\n");

    this->ReadSerial();
}

/* ===================================
 *      Private Functions
 * ====================================
 */
void Configuration::ReadSerial()
{
    int8_t idx = 0;
    while (true)
    {
        // Read serial data into this->serialCommandBuffer
        // In the case of a \r, then check the command against known commands
        // and call the corresponding function
        if (Serial.available() > 0)
        {
            char readByte = Serial.read();
            serialCommandBuffer[idx++] = readByte;
            if (idx > bufSize)
            {
                // do something about a buffer overflow
            }

            if (readByte == '\r')
            {
                // parse the first part of the command
                char *tok = ParseTok(this->serialCommandBuffer);
                if (strcmp(tok, "getNId") == 0)
                    this->cmdGetNodeId();
                else if(strcmp(tok, "getBId") == 0)
                    this->cmdGetBaseId();
                else if(strcmp(tok, "getNType") == 0)
                    this->cmdGetType();
                else if(strcmp(tok, "setNId") == 0)
                    this->cmdSetNodeId();
                else if(strcmp(tok, "setBId") == 0)
                    this->cmdSetBaseId();
                else if(strcmp(tok, "setNType") == 0)
                    this->cmdSetType();
                idx = 0;
            }
        }
    }
}

/* ===================================
 *      Command Parsing
 * ====================================
 */

// strtok wrapper with the right deliminators
char *Configuration::ParseTok(char *buf)
{
    return strtok(buf, " \r\n");
}

int16_t Configuration::ParseNumber(char* buf)
{
    int len = strlen(buf);
    if(len > 0 && len <= 3) // 8 bit numbers can only be 3 chars max
    {
        // Make sure all characters are a valid digit
        for(auto i = 0; i < len; ++i)
        {
            if(buf[i] < '0' || buf[i] > '9')
                return -1;
        }
        return atoi((char *)buf);
    }
    return -1;
}

int16_t Configuration::ParseNumArg()
{
    auto arg = ParseTok(NULL);
    if (arg != NULL)
        return ParseNumber(arg);
    return -1;
}

void Configuration::SendSerialNum(int num)
{
    Serial.print(num);
    Serial.print('\r');
}


/* ===================================
 *      COMMANDS
 * ====================================
 */

// ========== Get Commands =============

void Configuration::cmdGetNodeId()
{
    SendSerialNum(currentNode.nodeId);
}

void Configuration::cmdGetBaseId()
{
    SendSerialNum(currentNode.baseId);
}

void Configuration::cmdGetType()
{
    SendSerialNum(currentNode.type);
}


// ========== Set Commands =============
// These commands will try to parse a number
// from the data. If they can't then they don't
// respond and the configurator will time out.
// Otherwise, they will send back the number they received

void Configuration::cmdSetNodeId()
{
    auto num = ParseNumArg();
    if (num != -1)
    {
        SetEEPromNodeId(num);
        this->currentNode.nodeId = num;
        SendSerialNum(currentNode.nodeId);
    }
}
void Configuration::cmdSetBaseId()
{
    auto num = ParseNumArg();
    if (num != -1)
    {
        SetEEPromBaseId(num);
        this->currentNode.baseId = num;
        SendSerialNum(currentNode.baseId);
    }
}
void Configuration::cmdSetType()
{
    auto num = ParseNumArg();
    if (num != -1)
    {
        SetEEPromNodeType(static_cast<NodeType>(num));
        this->currentNode.type = static_cast<NodeType>(num);
        SendSerialNum(currentNode.type);
    }
}

/* ===================================
 *      EEPROM
 * ====================================
 */

void Configuration::SetEEPromNodeId(uint8_t id)
{
    EEPROM.write(EEPROM_NodeIdOffset, id);
}
void Configuration::SetEEPromBaseId(uint8_t id)
{
    EEPROM.write(EEPROM_BaseIdOffset, id);
}
void Configuration::SetEEPromNodeType(NodeType type)
{
    EEPROM.write(EEPROM_NodeTypeOffset, type);
}

uint8_t Configuration::GetEEPromNodeId()
{
    return EEPROM.read(EEPROM_NodeIdOffset);
}
uint8_t Configuration::GetEEPromBaseId()
{
    return EEPROM.read(EEPROM_BaseIdOffset);
}
Configuration::NodeType Configuration::GetEEPromNodeType()
{
    return static_cast<NodeType>(EEPROM.read(EEPROM_NodeTypeOffset));
}
