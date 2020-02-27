#ifndef DISASTERRADIO_H
#define DISASTERRADIO_H

#include <Arduino.h>
#include "../DisasterClient.h"
#include "../DisasterServer.h"
#include "../DisasterMiddleware.h"

class DisasterRadio : public DisasterServer
{
	// std::list<DisasterClient *> connected;
	// DisasterClient * connected[10] = {0};

public:
	//  std::set<DisasterClient *> clients;
	// DisasterClient *clientsCon[10] = {0};

	// NOTE: multimethod so we can return the correct parameter type to chain on
	DisasterMiddleware *connect(DisasterMiddleware *client);
	DisasterClient *connect(DisasterClient *client);

	void disconnect(DisasterClient *client);

	void setup();
	void loop();

	void transmit(DisasterClient *client, struct Datagram datagram, size_t len);

	size_t size(void);

	uint8_t numClient = 0;
	uint8_t numConnections = 0;
};

#endif
