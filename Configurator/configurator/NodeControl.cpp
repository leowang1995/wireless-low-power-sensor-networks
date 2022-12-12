#include "NodeControl.h"

NodeControl::NodeControl(uint8_t rstPin) : connectedNode(), connected(false), resetPin(rstPin)
{
    Serial.begin(baud);

    // Set pin to Tri-state
    // to avoid problems
    pinMode(resetPin, INPUT);
    digitalWrite(resetPin, LOW);
  
    Serial.setTimeout(1000);
}


int16_t NodeControl::getNodeId()
{
    Serial.print(F("getNId\r"));
    auto num = readNodeRetNum();
    connectedNode.nodeId = num == -1 ? 0 : num;
    return num;
}

int16_t NodeControl::getBaseId()
{
    Serial.print(F("getBId\r"));
    auto num = readNodeRetNum();
    connectedNode.baseId = num == -1 ? 0 : num;
    return num;
}

NodeControl::Node::NodeType NodeControl::getNodeType()
{
    Serial.print(F("getNType\r"));
    connectedNode.type = static_cast<Node::NodeType>(readNodeRetNum());
    return connectedNode.type;
}

int16_t NodeControl::setNodeId(uint8_t id)
{
    char buf[16];
    sprintf(buf, "setNId %d\r", id);
    Serial.print(buf);
    auto num = readNodeRetNum();
    if(num != -1)
        connectedNode.nodeId = num;
    return num;
}

int16_t NodeControl::setBaseId(uint8_t id)
{
    char buf[16];
    sprintf(buf, "setBId %d\r", id);
    Serial.print(buf);
    auto num = readNodeRetNum();
    if(num != -1)
        connectedNode.baseId = num;
    return num;
}

NodeControl::Node::NodeType NodeControl::setNodeType(Node::NodeType nt)
{
    char buf[16];
    sprintf(buf, "setNType %d\r", nt);
    Serial.print(buf);
    auto type = static_cast<NodeControl::Node::NodeType>(readNodeRetNum());
    if(type != Node::invalid)
        connectedNode.type = type;
    return type;
}

char * NodeControl::concatNodeTypeString(char* buf)
{
    switch(connectedNode.type)
    {
        case Node::sensor: 
            strcat(buf, "SN");
            break;
        case Node::forwarder:
            strcat(buf, "FW");
            break;
        case Node::base:
            strcat(buf, "BA");
            break;
        default:
        break;
    }
    return buf;
}

bool NodeControl::isConnected()
{
    return this->connected;
}

bool NodeControl::connectToNode()
{
    this->connected = false;
    // Connecting consists of resetting the node while holding the tx line high
    // then waiting until a CRLF is received.
    // Timeout after 1000ms

    // Reset
    Serial.end();
    pinMode(txPin, OUTPUT);
    digitalWrite(txPin, HIGH);
    pinMode(resetPin, OUTPUT); // Normally kept as input for high-impedance
    delay(30);
    pinMode(resetPin, INPUT);
    delay(30); // Node takes around 3.3ms to reset
    digitalWrite(txPin, LOW); // Node will wait until its RX goes low, then will delay for 50ms before sending anything
    Serial.begin(baud);
    // Just read through characters until the last chars are a CRLF
    if(Serial.find((char *)"\r\n", 2))
    {
        if(getNodeId() != -1 &&
           getBaseId() != -1 &&
           getNodeType() != -1)
            this->connected = true;
    }
    return this->connected;
}

void NodeControl::disconnect()
{
    // Simply reset the target
    pinMode(resetPin, OUTPUT);
    delay(30);
    pinMode(resetPin, INPUT);
}

/// Tries to read a number that the node will return from serial.
/// Expects only a number and the terminator will be sent over serial.
/// A number from the node will be terminated by a CR and 8 bits unsigned.
/// If a number can't be read, -1 is returned.
int16_t NodeControl::readNodeRetNum()
{
    uint8_t buf[16];
    int len = Serial.readBytesUntil('\r', buf, 16);

    if(len > 0 && len <= 3) // 8 bit numbers can only be 3 chars max
    {
        // Make sure all characters are a valid digit
        for(int i = 0; i < len; ++i)
        {
            if(buf[i] < '0' || buf[i] > '9')
                return -1;
        }
        buf[len] = '\0';
        return atoi((char *)buf);
    }
    return -1;
}
