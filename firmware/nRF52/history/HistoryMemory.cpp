#include <list>

#include "HistoryMemory.h"
#include "LoRalayer2.h"

std::list<String> buffer;

/** Current line used */
uint8_t currentLine = 0;

void HistoryMemory::record(struct Datagram datagram, size_t len)
{
	if (datagram.type != 'c')
	{
		// Serial.println("Not a chat message, skip");
		return;
	}

	char tempBuf[len - DATAGRAM_HEADER + 1] = {0};
	memcpy(tempBuf, datagram.message, len);
	String newEntry = String(tempBuf);

	//   Serial.printf("HistoryMemory::record New History entry >%s<\n", newEntry.c_str());
	//   printMem();

	buffer.push_back(newEntry);
	while (buffer.size() > limit)
	{
		buffer.pop_front();
	}

	//   Serial.printf("HistoryMemory::record New History entry >%s<\n", newEntry.c_str());

	// if (currentLine == DEFAULT_LIMIT)
	// {
	// 	// Buffer is full, shift text one line up
	// 	for (int idx = 0; idx < DEFAULT_LIMIT; idx++)
	// 	{
	// 		buffer[idx] = buffer[idx + 1];
	// 	}
	// 	currentLine--;
	// }
	// buffer[currentLine] = newEntry;
	// if (currentLine != DEFAULT_LIMIT)
	// {
	// 	currentLine++;
	// }
}

void HistoryMemory::replay(DisasterClient *client)
{
	for (String message : buffer)
	{
		// Serial.printf("HistoryMemory::replay Replay >%s<\n", message.c_str());

		Datagram newHistory = {0};
		memcpy(newHistory.destination, LL2.broadcastAddr(), ADDR_LENGTH);
		newHistory.type = 'c';
		size_t msgLen = sprintf((char *)newHistory.message, message.c_str());

		client->receive(newHistory, msgLen + DATAGRAM_HEADER);
		// Give others a chance to process the message
		delay(200);
	}
}
