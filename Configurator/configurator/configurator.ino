
#include "Button.h"
#include "Display.h"
#include "NodeControl.h"

int intPow(int num, int pow);
int numSetScreen(char *text, int dispNum, int numDigits, int min, int max);
void setupButtons();
void pollButtons();

Display display;
Button upButton(2);
Button downButton(3);
Button confButton(4);
const uint8_t rstPin = 8;

void setup()
{
  Serial.begin(9600);
  display.setupDisplay();
  setupButtons();

  // Right after startup, show the logo for a bit
  display.displayULogo();
  delay(750);
}

const char *connectedIdleOptions[] = {"Set ID", "Set Type", "Set BaseID", "Disconnect"};
const char *nodeTypes[] = {"SensorNode", "Forwarder", "Base"};
void loop()
{
  enum State
  {
    st_WaitForConnect,
    st_Connect,
    st_ConnectedIdle,
    st_SetId,
    st_SetNodeType,
    st_SetBaseId
  };

  State currentState = st_WaitForConnect;
  State prevState = currentState;
  unsigned long lastPollTime = 0;
  auto buttonsPolled = false;
  auto firstPass = true; // First pass of a new state
  auto nc = NodeControl{rstPin};

  while (true)
  {
    // Poll the buttons and set some state vaiables
    if (millis() - lastPollTime >= 2) // Try to do this around once every 2 ms
    {
      pollButtons();
      buttonsPolled = true;
    }

    // Here begins the state machine
    // Has states for connecting to a node,
    // setting a node Id, setting the type and more
    switch (currentState)
    {
    // ===== Wait for the user to connect =====
    case st_WaitForConnect:
      if (firstPass) // Don't want to draw every time
        display.displayString(F("Press any button to connect"));

      if (buttonsPolled && (upButton.pressed() || downButton.pressed() || confButton.pressed()))
        currentState = st_Connect;
      break;

    // ===== Try to connect to a node =====
    case st_Connect:
      display.displayString(F("Connecting..."));
      if (nc.connectToNode())   // Worked
        currentState = st_ConnectedIdle;
      else
      {
        display.displayString(F("Couldn't\r\nconnect"));
        delay(1000);                      // lazy
        currentState = st_WaitForConnect; // Go back to try to connect
      }
      break;

    // ===== Connected to node and sitting at the select screen =====
    case st_ConnectedIdle:
    {
      char buf[10];
      sprintf(buf, "Node: %03d", nc.connectedNode.nodeId);
      auto selection = textSelectScreen(connectedIdleOptions, sizeof(connectedIdleOptions) / sizeof(*connectedIdleOptions), buf);
      if (selection == 0)
        currentState = st_SetId;
      else if (selection == 1)
        currentState = st_SetNodeType;
      else if (selection == 2)
        currentState = st_SetBaseId;
      else if (selection == 3)
      {
        nc.disconnect();
        currentState = st_WaitForConnect;
      }
      break;
    }

    // ===== Set the node ID =====
    case st_SetId:
    {
      auto setNum = numSetScreen("Node ID:\r\n", nc.connectedNode.nodeId, 3, 0, 250);
      if(nc.setNodeId(setNum) == -1)
      {
        display.displayString(F("Couldn't\r\nset ID"));
        delay(1000); // lazy
      }
      currentState = st_ConnectedIdle;
      break;
    }

    // ===== Set the node type (sense, forwarder, base) =====
    case st_SetNodeType:
    {
      char buf[10] = "Type: ";
      nc.concatNodeTypeString(buf);
      auto type = static_cast<NodeControl::Node::NodeType>(textSelectScreen(nodeTypes, 3, buf));
      if(nc.setNodeType(type) == NodeControl::Node::invalid)
      {
        display.displayString(F("Couldn't\r\nset type"));
        delay(1000);
      }
      currentState = st_ConnectedIdle;
      break;
    }

    case st_SetBaseId:
    {
      auto setNum = numSetScreen("Base ID:\r\n", nc.connectedNode.baseId, 3, 0, 250);
      if(nc.setBaseId(setNum) == -1)
      {
        display.displayString(F("Couldn't\r\nset BaseID"));
        delay(1000);
      }
      currentState = st_ConnectedIdle;
      break;
    }

    default:
      currentState = st_WaitForConnect;
    }

    // Various state machine handling
    if (firstPass)
      firstPass = false;

    if (currentState != prevState)
    {
      firstPass = true;
      prevState = currentState;
    }
    buttonsPolled = false;
  }
}

// Power function but for integers
// (to avoid floating point issues, also probably more performant)
int intPow(int num, int pow)
{
  int ret = 1;
  for (int i = 0; i < pow; ++i)
    ret *= num;

  return ret;
}

/// Shows a screen that allows you to set a number.
/// Returns the number that was set.
/// Doesn't display well with negative numbers
int numSetScreen(const char *text, int dispNum, int numDigits, int min, int max)
{
  int highlightPos = 0;
  char buf[20];      // Used for display
  char formatBuf[8]; // Used for formatting the string
  auto updateDisp = true;
  auto textLen = strlen(text);
  // replaces a variable width in sprintf e.g. sprintf(buf, "%0*d", numDigits) as avr-gcc doesn't implement it
  sprintf(formatBuf, "%%s%%0%dd", numDigits);

  unsigned long lastPollTime = 0;
  while (highlightPos < numDigits)
  {
    if (millis() - lastPollTime >= 2) // Try to do this around once every 2 ms
    {
      pollButtons();
      lastPollTime = millis();

      if (upButton.pressed())
      {
        // adds 1 to the correct digit position
        // pos 0 is the first, largest magnitude digit
        dispNum += intPow(10, numDigits - 1 - highlightPos);
        if (dispNum > max)
          dispNum = max;
        updateDisp = true;
      }
      else if (downButton.pressed())
      {
        dispNum -= intPow(10, numDigits - 1 - highlightPos);
        if (dispNum < min)
          dispNum = min;
        updateDisp = true;
      }
      else if (confButton.pressed())
      {
        ++highlightPos;
        updateDisp = true;
      }

      if (updateDisp)
      {
        // equivalent to sprintf(buf, "%s%0*d", text, numDigits, dispNum)
        sprintf(buf, formatBuf, text, dispNum);
        display.displayHighlightedString(buf, textLen + highlightPos, 1);
        updateDisp = false;
      }
    }
  }
  return dispNum;
}

/// Shows a screen of the texts
/// Each text can only be 10 characters (one full line of size 2 font)
/// Can add a header to the top of the select list. If it is blank then it will not take up space.
///
/// Returns the index of the text that was selected
int8_t textSelectScreen(const char **texts, uint8_t numTexts, const char *header)
{
  int8_t selectedItem = 0;
  auto updateDisp = true;
  auto useHeader = strlen(header) > 0;

  unsigned long lastPollTime = 0;
  while (true)
  {
    if (millis() - lastPollTime >= 2) // Try to do this around once every 2 ms
    {
      pollButtons();
      lastPollTime = millis();

      if (upButton.pressed())
      {
        if (--selectedItem < 0)
          selectedItem = 0;
        updateDisp = true;
      }
      else if (downButton.pressed())
      {
        if (++selectedItem >= numTexts - 1)
          selectedItem = numTexts - 1;
        updateDisp = true;
      }
      else if (confButton.pressed())
      {
        return selectedItem;
      }

      if (updateDisp)
      {
        if (useHeader)
          display.displayTextLinesWithheader(texts, numTexts, selectedItem, header);
        else
          display.displayTextLines(texts, numTexts, selectedItem);

        updateDisp = false;
      }
    }
  }
}

void setupButtons()
{
  upButton.init();
  downButton.init();
  confButton.init();
}

void pollButtons()
{
  upButton.read();
  downButton.read();
  confButton.read();
}
