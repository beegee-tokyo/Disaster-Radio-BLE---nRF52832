
#include <list>
#include <set>
using namespace std;

#include "DisasterRadio.h"

std::list<DisasterClient *> connected;
std::set<DisasterClient *> clientsCon;

size_t DisasterRadio::size(void) {
	// return numClient;
	return clientsCon.size();
}

// NOTE: multimethod so we can return the correct parameter type to chain on
DisasterMiddleware *DisasterRadio::connect(DisasterMiddleware *client)
{
	client->server = this;
	clientsCon.insert(client);
	connected.push_back(client);
	// clientsCon[numClient] = client;
	// connected[numClient] = client;
	// numClient++;
	// numConnections++;
	// Serial.printf("Connect: Number of clients: %d connections %d\n", numClient, numConnections);
	return client;
}
DisasterClient *DisasterRadio::connect(DisasterClient *client)
{
	client->server = this;
	clientsCon.insert(client);
	connected.push_back(client);
	// clientsCon[numClient] = client;
	// connected[numClient] = client;
	// numClient++;
	// numConnections++;
	// Serial.printf("Connect: Number of clients: %d connections %d\n", numClient, numConnections);
	return client;
}
void DisasterRadio::disconnect(DisasterClient *client)
{
	// for (int idx = 0; idx < numClient; idx++)
	// {
	// 	if (clientsCon[idx] == client)
	// 	{
	// 		for (int idx2 = idx; idx2 < numClient; idx2++)
	// 		{
	// 			clientsCon[idx] = clientsCon[idx + 1];
	// 		}
	// 		clientsCon[numClient - 1] = 0;
	// 		numClient--;
	// 	}
	// }
	// Serial.printf("Disconnect: Number of clients: %d connections %d\n", numClient, numConnections);

	clientsCon.erase(client);
	delete client;
}

void DisasterRadio::transmit(DisasterClient *client, struct Datagram datagram, size_t len)
{
	for (DisasterClient *other_client : clientsCon)
	{
		if (client != other_client)
		{
			other_client->receive(datagram, len);
		}
	}
}

void DisasterRadio::setup()
{
}

void DisasterRadio::loop()
{
	for (DisasterClient *client : connected)
	{
		client->setup();
	}
	connected.clear();

	for (DisasterClient *client : clientsCon)
	{
		client->loop();
	}

	// for (int idx = 0; idx < numConnections; idx++)
	// {
	// 	connected[idx]->setup();
	// 	connected[idx] = 0;
	// }
	// numConnections = 0;
	// for (int idx = 0; idx < numClient; idx++)
	// {
	// 	clientsCon[idx]->loop();
	// }
}
