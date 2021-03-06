
#include "StreamClient.h"

#define STREAM_ECHO

void StreamClient::setup()
{
	stream->setTimeout(1000);
}

void StreamClient::loop()
{
	if (stream->available() > 0)
	{
		String message = "";
		while (stream->available() > 0)
		{
			message += stream->readString();
		}
		stream->flush();
		// String message = stream->readString();
		Serial.printf("StreamClient::loop received %s\n", message.c_str());
		size_t len = message.length();
		uint8_t data[len];
		message.getBytes(data, len);
		struct Datagram datagram = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
		memset(datagram.message, 0, 233);
		datagram.type = 'c';
		memcpy(datagram.message, data, len);
		len = len + DATAGRAM_HEADER;

		server->transmit(this, datagram, len);
	}
};

void StreamClient::receive(struct Datagram datagram, size_t len)
{
	stream->write(datagram.message, len - DATAGRAM_HEADER);
};
