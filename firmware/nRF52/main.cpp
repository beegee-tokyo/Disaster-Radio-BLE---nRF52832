#include <Arduino.h>

// required for LoRa and/or AsyncSDServer libraries?
#ifndef NRF52
#include <FS.h>
#endif
#include <SPI.h>

// #include <SPIFFS.h>

#include "configDR.h"

// server
#include "server/DisasterRadio.h"
// client
// #include "client/OLEDClient.h"
#include "client/HistoryRecord.h"
#include "client/LoRaClient.h"
#include "client/StreamClient.h"
// #include "client/GPSClient.h"
#ifndef NRF52
#include "client/BleUartClient.h"
#else
#include "bluefruit.h"
#include "client/BleUartClientNRF52.h"
#endif

#ifdef USE_SX1262
#include <SX126x-Arduino.h>
#endif

// middleware
#include "middleware/Console.h"
#include "middleware/HistoryReplay.h"
// history
#include "history/HistoryMemory.h"

#include "DisasterHistory.h"
#include "DisasterMiddleware.h"

#define MDNS_NAME_PREFIX "disaster"
#define HOST_NAME "disaster.radio"
#define WIFI_AP_SSID_PREFIX "disaster.radio "

#define INDEX_FILE "index.htm"

BleUartClient ble_client;

#include "settings/settings.h"

DisasterRadio *radio = new DisasterRadio();
DisasterHistory *history = NULL;

int internal_clients = 0;
int clients = 0;
int routes = 0;

#define WIFI_POLL_DELAY 500
#define WIFI_POLL_TRIES 10

char macaddr[12 + 1] = {'\0'};
char ssid[100] = {'\0'};

bool sdInitialized = false;
bool spiffsInitialized = false;
bool displayInitialized = false;
bool loraInitialized = false;

void setupHistory()
{
	Serial.println("* Initializing history...");

	history = new HistoryMemory();

	radio->connect(new HistoryRecord(history));
}

struct Datagram buildDatagram(uint8_t destination[ADDR_LENGTH], uint8_t type, uint8_t *data, size_t len)
{
	struct Datagram datagram;
	memcpy(datagram.destination, destination, ADDR_LENGTH);
	datagram.type = 'c';
	memcpy(datagram.message, data, len);
	return datagram;
}

class WelcomeMessage : public DisasterMiddleware
{
public:
	void setup()
	{
		char data[128] = {0};
		int msgLen = 0;
		Serial.printf("WelcomeMessage username: >%s<\n", username.c_str());
		if (username.length() == 0)
		{
			msgLen = sprintf(data, "00c|Welcome to DISASTER RADIO ");
		}
		else
		{
			msgLen = snprintf(data, 127, "00c|Welcome to DISASTER RADIO >%s<", username.c_str());
		}

		struct Datagram datagram1 = buildDatagram(LL2.broadcastAddr(), 'c', (uint8_t *)data, msgLen);
		client->receive(datagram1, msgLen + DATAGRAM_HEADER);
		if (!sdInitialized)
		{
			msgLen = sprintf(data, "00c|WARNING: SD card not found, functionality may be limited");
			struct Datagram datagram2 = buildDatagram(LL2.broadcastAddr(), 'c', (uint8_t *)data, msgLen);
			client->receive(datagram2, msgLen + DATAGRAM_HEADER);
		}
		if (!loraInitialized)
		{
			msgLen = sprintf(data, "00c|WARNING: LoRa radio not found, functionality may be limited");
			struct Datagram datagram3 = buildDatagram(LL2.broadcastAddr(), 'c', (uint8_t *)data, msgLen);
			client->receive(datagram3, msgLen + DATAGRAM_HEADER);
		}
		client->setup();
	}
};

void setupSerial()
{
	Serial.println("* Initializing serial...");

	radio->connect(new WelcomeMessage())
		->connect(new HistoryReplay(history))
		->connect(new Console())
		->connect(new StreamClient(&Serial));
}

void setupLoRa()
{
#ifdef LORA_CS
#ifdef USE_SX1262
	Serial.println("* Initializing SX1262 LoRa...");
#else
	Serial.println("* Initializing LoRa...");
#endif

	Layer1.setPins(LORA_CS, LORA_RST, LORA_IRQ);
	LL2.setLocalAddress(macaddr);
	if (Layer1.init())
	{
		LoRaClient *lora_client = new LoRaClient();
		if (lora_client->init())
		{
			Serial.printf(" --> LoRa address: %s\n", macaddr);
			radio->connect(lora_client);
			loraInitialized = true;
			return;
		}
	}
	Serial.println(" --> Failed to initialize LoRa");
#endif
}

#ifndef NRF52
#define STATUS_BAR_HEIGHT 12
#define MARGIN 3

extern char apName[];

void drawStatusBar()
{
	if (!displayInitialized)
	{
		return;
	}

	display.setFont(ArialMT_Plain_10);

	// clear the status bar
	display.setColor(BLACK);
	display.fillRect(0, 0, OLED_WIDTH, STATUS_BAR_HEIGHT);

	// draw MAC
	display.setColor(WHITE);
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	if (!useBLE)
	{
		display.drawString(0, 0, macaddr);
	}
	else
	{
		display.drawString(0, 0, apName);
	}

	String clientsString = String(clients) + "," + routes;
	String ipString = "";
	if (!useBLE)
	{
		ipString = ip.toString();
	}

	// measure client and IP width
	int widthClients = (int)display.getStringWidth(clientsString) + 1;
	int widthIP = display.getStringWidth(ipString);

	// clear space behind IP address in case the SSID was too long
	display.setColor(BLACK);
	display.fillRect(OLED_WIDTH - widthClients - MARGIN - widthIP - MARGIN, 0, OLED_WIDTH, STATUS_BAR_HEIGHT);

	// draw client count
	display.setColor(WHITE);
	display.fillRect(OLED_WIDTH - widthClients, 2, widthClients, STATUS_BAR_HEIGHT - 3);
	display.setColor(BLACK);
	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(OLED_WIDTH, 0, clientsString);

	// draw IP address
	display.setColor(WHITE);
	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(OLED_WIDTH - widthClients - MARGIN, 0, ipString);

	// draw divider line
	display.drawLine(0, 12, 128, 12);
	display.display();
}

void setupDisplay()
{
#ifdef OLED_SDA
	Serial.println("* Initializing display...");
	if (OLED_RST != NOT_A_PIN)
	{
		pinMode(OLED_RST, OUTPUT);
		digitalWrite(OLED_RST, LOW); // set GPIO16 low to reset OLED
		delay(50);
		digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high
	}
	if (display.init())
	{
		displayInitialized = true;

		display.flipScreenVertically();
		display.clear();

		radio->connect(new OLEDClient(&display, 0, STATUS_BAR_HEIGHT + 1));

		drawStatusBar();
	}
	else
	{
		Serial.println(" --> Failed to initialize the display");
	}

#endif
}

void setupGPS()
{
#ifdef GPS_SERIAL
	Serial.println("* Initializing GPS...");
	Serial1.begin(GPS_SERIAL);
	radio->connect(new GPSClient(&Serial1));
#endif
}
#endif

typedef volatile uint32_t REG32;
// #define pREG32 (REG32 *)
#define pREG32 (uint32_t *)

// #define MAC_ADDRESS_HIGH (*(pREG32(0x100000a8)))
// #define MAC_ADDRESS_LOW (*(pREG32(0x100000a4)))

#define MAC_ADDRESS_HIGH (*((uint32_t *)(0x100000a8)))
#define MAC_ADDRESS_LOW (*((uint32_t *)(0x100000a4)))

void setupBLE(void)
{
	Serial.println("* Initializing BLE...");

	uint32_t addr_high = ((MAC_ADDRESS_HIGH)&0x0000ffff) | 0x0000c000;
	uint32_t addr_low = MAC_ADDRESS_LOW;
	sprintf(macaddr, "%02X%02X%02X%02X%02X%02X",
			(uint8_t)(addr_high), (uint8_t)(addr_high >> 8), (uint8_t)(addr_low),
			(uint8_t)(addr_low >> 8), (uint8_t)(addr_low >> 16), (uint8_t)(addr_low >> 24));

	ble_client.startServer([](BleUartClient *ble_client) {
		radio->connect(new WelcomeMessage())
			->connect(new HistoryReplay(history))
			->connect(ble_client);
	});
}

void setup()
{
	Serial.begin(115200);
	delay(200);

	Serial.printf("Free Heap after Serial.begin(): %d\n", dbgHeapFree());
	// Get saved settings
	getSettings();
	Serial.printf("Free Heap after getSettings(): %d\n", dbgHeapFree());

	setupHistory();
	Serial.printf("Free Heap after setupHistory(): %d\n", dbgHeapFree());
	setupSerial();
	Serial.printf("Free Heap after setupSerial(): %d\n", dbgHeapFree());
	setupBLE();
	Serial.printf("Free Heap after setupBLE(): %d\n", dbgHeapFree());

	setupLoRa();
	Serial.printf("Free Heap after setupLoRa(): %d\n", dbgHeapFree());
#ifdef HAS_DISPLAY
	setupDisplay();
#endif
	//   setupGPS();

	internal_clients = radio->size();
	// internal_clients = radio->numClient;

	radio->setup();
}

void loop()
{
	radio->loop();

	int new_clients = radio->size() - internal_clients;
	// int new_clients = radio->numClient - internal_clients;
	int new_routes = LL2.getRouteEntry();
	if (clients != new_clients || routes != new_routes)
	{
		clients = new_clients;
		routes = new_routes;
		Serial.printf("--> clients=%d routes=%d\n", clients, routes);
#ifndef NRF52
		drawStatusBar();
#endif
	}
}
