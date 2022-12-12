/// Contains the logic for getting and setting ids and types of the node
/// and 
#pragma once
#include <Arduino.h>
class NodeControl
{
public:
    struct Node
    {
        enum NodeType
        {
            sensor,
            forwarder,
            base,
            invalid = -1
        };
        uint8_t nodeId;
        uint8_t baseId;
        NodeType type;
    } connectedNode;

    NodeControl(uint8_t rstPin);
    int16_t getNodeId();
    int16_t getBaseId();
    Node::NodeType getNodeType();
    int16_t setNodeId(uint8_t id);
    int16_t setBaseId(uint8_t id);
    Node::NodeType setNodeType(Node::NodeType nt);
    char *concatNodeTypeString(char* buf);
    bool isConnected();
    bool connectToNode();
    void disconnect();


private:
    bool connected;
    uint8_t resetPin;
    int16_t readNodeRetNum();
    const int txPin = 1;
    const int baud = 9600;
};
