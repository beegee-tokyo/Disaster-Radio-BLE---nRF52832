#include "Console.h"

/** Comment out to stop debug output */
// #define DEBUG_OUT

#include <Layer1.h>
#include <LoRaLayer2.h>
#include "settings/settings.h"

#include <vector>

#define START_MESSAGE "Type '/join NICKNAME' to join the chat, or '/help' for more commands."

void Console::setup()
{
  DisasterMiddleware::setup();
  if (history)
  {
    history->replay(this);
  }
  struct Datagram response;
  int msgLen = sprintf((char *)response.message, "Type '/join NICKNAME' to join the chat, or '/help' for more commands.\n");
  client->receive(response, msgLen + DATAGRAM_HEADER);
}

void Console::processLine(char *message, size_t len)
{
  struct Datagram response;
  memset(response.message, 0, 233);
  int msgLen;

#ifdef DEBUG_OUT
  Serial.printf("Console::processLine help result %s\n", message);
#endif
  // message might not be NULL ended
  char msgBuff[len + 2] = {0};
  memcpy(msgBuff, message, len);

  if (msgBuff[0] == '/')
  {
    std::vector<char *> args;

    char *p;

    p = strtok(msgBuff, " ");
    while (p)
    {
      args.push_back(p);
      p = strtok(NULL, " ");
    }

    p = args[0];

    if (strncmp(&args[0][1], "help", 4) == 0)
    {
      msgLen = sprintf((char *)response.message, "Commands: /help /join /nick /del /raw /lora /switch /restart\n");
      client->receive(response, msgLen + DATAGRAM_HEADER);
#ifdef DEBUG_OUT
      Serial.printf("Console::processLine help result %s\n", (char *)response.message);
#endif
    }
    else if (strncmp(&args[0][1], "raw", 3) == 0)
    {
#ifdef DEBUG_OUT
      Serial.printf("Console::processLine switching to RAW\n");
#endif
      disconnect(client);
      server->disconnect(this);
      server->connect(client);
    }
    else if (strncmp(&args[0][1], "switch", 6) == 0)
    {
#ifdef DEBUG_OUT
      Serial.printf("Switching UI to %s\n", useBLE ? "WiFi" : "BLE");
#endif
      saveUI(!useBLE);
      delay(500);
      ESP.restart();
    }
    else if (((strncmp(&args[0][1], "join", 4) == 0) || (strncmp(&args[0][1], "nick", 4) == 0)) && (args.size() > 1))
    {
      // Trim username to remove trailing CR and LF's
      int idx = 0;
      while (args[1][idx] != '0')
      {
        if ((args[1][idx] == 0x0a) || (args[1][idx] == 0x0d))
        {
          args[1][idx] = 0;
          break;
        }
        idx++;
      }
      if (username.length() > 0)
      {
        msgLen = sprintf((char *)response.message, "00c|~ %s is now known as %s\n", username.c_str(), args[1]);
      }
      else
      {
        msgLen = sprintf((char *)response.message, "00c|~ %s joined the channel\n", args[1]);
      }
      memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
      response.type = 'c';
      server->transmit(this, response, msgLen + DATAGRAM_HEADER);
      memcpy(response.message, &response.message[4], msgLen - 4);
      response.message[msgLen - 4] = '\n';
      client->receive(response, msgLen - 4 + DATAGRAM_HEADER);
      username = String(args[1]);
      saveUsername(username);

#ifdef DEBUG_OUT
      Serial.printf("Console::processLine join/nick result %s\n", (char *)response.message);
      Serial.printf("Console::processLine new username is %s\n", username.c_str());
#endif
    }
    else if (strncmp(&args[0][1], "del", 4) == 0)
    {
      // Delete the saved username
      username = "";
      saveUsername("");
    }
    else if ((strncmp(&args[0][1], "restart", 7) == 0))
    {
#ifdef DEBUG_OUT
      Serial.printf("Console::processLine restarting\n");
      delay(500);
#endif
      ESP.restart();
    }
    else if ((strncmp(&args[0][1], "lora", 4) == 0))
    {
      RoutingTableEntry *tblPtr;
      int routeTableSize = LL2.getRouteEntry();
      Serial.printf("RoutingTableSize = %d", routeTableSize);
      Datagram loraInfo;
      memset((char *)loraInfo.message, 0, 233);
      memcpy(loraInfo.destination, LL2.broadcastAddr(), ADDR_LENGTH);
      loraInfo.type = 'c';
      size_t msgLen = 0;

      strcat((char *)loraInfo.message, "Address: ");
      msgLen += 9;
      uint8_t *localAdr = LL2.localAddress();
      char tempBuf[64];
      for (int i = 0; i < ADDR_LENGTH; i++)
      {
        msgLen += sprintf(tempBuf, "%02X", localAdr[i]);
        strcat((char *)loraInfo.message, tempBuf);
      }
      strcat((char *)loraInfo.message, "\n");
      msgLen++;
      msgLen += sprintf(tempBuf, "Routing Table: total routes %d\r\n", LL2.getRouteEntry());
      strcat((char *)loraInfo.message, tempBuf);
      for (int i = 0; i < routeTableSize; i++)
      {
        tblPtr = LL2.getRoutingTableEntry(i);
        msgLen += sprintf(tempBuf, "%d hops from ", tblPtr->distance);
        strcat((char *)loraInfo.message, tempBuf);
        for (int j = 0; j < ADDR_LENGTH; j++)
        {
          msgLen += sprintf(tempBuf, "%02X", tblPtr->destination[j]);
          strcat((char *)loraInfo.message, tempBuf);
        }
        strcat((char *)loraInfo.message, " via ");
        msgLen += 5;
        for (int j = 0; j < ADDR_LENGTH; j++)
        {
          msgLen += sprintf(tempBuf, "%02X", tblPtr->nextHop[j]);
          strcat((char *)loraInfo.message, tempBuf);
        }
        msgLen += sprintf(tempBuf, " metric %3d ", tblPtr->metric);
        strcat((char *)loraInfo.message, tempBuf);
        strcat((char *)loraInfo.message, "\n");
        msgLen++;
      }

      client->receive(loraInfo, msgLen + DATAGRAM_HEADER);

      // TODO: print to client instead of serial
      // Serial.printf("Address: ");
      // LL2.printAddress(LL2.localAddress());
      // LL2.printRoutingTable();
    }
    else
    {
      msgLen = sprintf((char *)response.message, "Unknown command '%s'\n", msgBuff);
      client->receive(response, msgLen + DATAGRAM_HEADER);
    }
  }
  else if (username.length() > 0)
  {
    msgLen = sprintf((char *)response.message, "00c|<%s>%s", username.c_str(), msgBuff);
    memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
    response.type = 'c';
    server->transmit(this, response, msgLen + DATAGRAM_HEADER);
    memcpy(response.message, &response.message[4], msgLen - 4);
    response.message[msgLen - 4] = '\n';
#ifdef DEBUG_OUT
    Serial.printf("Console message =>%s<\n", &response.message[4]);
#endif
    client->receive(response, msgLen - 3 + DATAGRAM_HEADER);
  }
  else
  {
    msgLen = sprintf((char *)response.message, "00c|%s", msgBuff);
    memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
    response.type = 'c';
    server->transmit(this, response, msgLen + DATAGRAM_HEADER);
    memcpy(response.message, &response.message[4], msgLen - 4);
    response.message[msgLen - 4] = '\n';
#ifdef DEBUG_OUT
    Serial.printf("Console message =>%s<\n", &response.message[4]);
#endif
    client->receive(response, msgLen - 3 + DATAGRAM_HEADER);
  }
}

void Console::transmit(DisasterClient *client, struct Datagram datagram, size_t len)
{
#ifdef DEBUG_OUT
  Serial.printf("CONSOLE::transmit raw data with len %d type %c\n", len, datagram.type);
  for (int idx = 0; idx < len - DATAGRAM_HEADER; idx++)
  {
    Serial.printf("%02X ", datagram.message[idx]);
  }
  Serial.println("\n" + String((char *)datagram.message));
#endif

  // Split in case it is multi line
  /// \todo handle CR-LF combination
  char *p;
  char *next;
  int processLen = len - DATAGRAM_HEADER;

  p = strtok((char *)datagram.message, "\n");

  while (p)
  {
    next = p;
    p = strtok(NULL, "\n");
    if (p)
    {
      processLine(next, p - next - 1);
      processLen -= p - next;
    }
    else
    {
      processLine(next, processLen - 1);
    }
  }
};

void Console::receive(struct Datagram datagram, size_t len)
{
  int msgSize = len - DATAGRAM_HEADER;

  if ((datagram.message[2] == 'c') && (datagram.message[3] == '|'))
  {
#ifdef DEBUG_OUT
    Serial.printf("CONSOLE::receive raw data with len %d type %c\n", msgSize, datagram.type);
    for (int idx = 0; idx < msgSize; idx++)
    {
      Serial.printf("%02X ", datagram.message[idx]);
    }
    Serial.println("");
#endif

    memcpy(datagram.message, &datagram.message[4], msgSize - 4);
    msgSize -= 4;
    datagram.message[msgSize] = '\n';
    msgSize += 1;
    client->receive(datagram, msgSize + DATAGRAM_HEADER);
  }
};
