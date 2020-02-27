#ifdef NRF52
#include "BleUartClientNRF52.h"
#include "settings/settings.h"

// OTA DFU service
BLEDfu bledfu;
// UART service
BLEUart bleuart;

extern char macaddr[];

/** Callback for client notification enable/disable */
void uartNotifyCallback(uint16_t connHandle, boolean status);
/** Callback for client connection */
void connect_callback(uint16_t conn_handle);
/** Callback for client disconnect */
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

/** Flag if device is connected */
bool deviceConnected = false;

/** Flag if we already registered on the server */
bool notifEnabled = false;

/** Unique device name */
char apName[] = "DR-xxxxxxxxxxxx";

uint8_t rxData[512] = {0};
size_t rxLen = 0;
bool dataRcvd = false;

extern BleUartClient ble_client;
void (*connectCallback)(BleUartClient *);

void BleUartClient::receive(struct Datagram datagram, size_t len)
{
	if (deviceConnected)
	{
#ifdef DEBUG_OUT
		Serial.printf("BLE:receive %s length %d\n", (char *)datagram.message, len - DATAGRAM_HEADER);
		Serial.println("BLE::receive raw data");
		for (int idx = 0; idx < len - DATAGRAM_HEADER; idx++)
		{
			Serial.printf("%02X ", datagram.message[idx]);
		}
		Serial.println("");
#endif

		unsigned char buf[len - DATAGRAM_HEADER] = {'\0'};
		memcpy(buf, &datagram.message, len - DATAGRAM_HEADER);

#ifdef DEBUG_OUT
		Serial.printf("BLE: Sending raw data with len %d\n", sizeof(buf));
		for (int idx = 0; idx < sizeof(buf); idx++)
		{
			Serial.printf("%02X ", buf[idx]);
		}
		Serial.println("");

		Serial.printf("BLE::receive %s len %d type %c\n", buf, sizeof(buf), datagram.type);
#endif

		bleuart.write(buf, sizeof(buf));

		// Give BLE time to get the data out
		delay(100);
	}
}

void BleUartClient::handleData(void *data, size_t len)
{
#ifdef DEBUG_OUT
	char debug[len] = {'\0'};
	memcpy(debug, data, len);
	Serial.println("BLE: Received raw data");
	for (int idx = 0; idx < len; idx++)
	{
		Serial.printf("%02X ", debug[idx]);
	}
	Serial.println("");
#endif

	char *msg = (char *)data;

	if (msg[2] == 'u')
	{
		if (msg[4] == '~')
		{
			// delete username
			username = "";
			saveUsername(username);
#ifdef DEBUG_OUT
			Serial.printf("Deleted username over BLE\n");
#endif
			return;
		}
		else
		{
			// username set over BLE
			username = String(&msg[4]);
			Serial.printf("Username set over BLE: %s\n", username.c_str());
			saveUsername(username);
			return;
		}
	}

	if (msg[2] == 'i')
	{
		// Switch to WiFi interface
#ifdef DEBUG_OUT
		Serial.printf("User wants to switch to WiFi");
#endif
		saveUI(false);
		delay(500);
		sd_nvic_SystemReset();
	}

	struct Datagram datagram = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	memset(datagram.message, 0, 233);
	datagram.type = msg[2];
	memcpy(&datagram.message, data, len);

#ifdef DEBUG_OUT
	Serial.printf("BLE::handleData %s len %d type %c\n", msg, len, datagram.type);
#endif

	server->transmit(this, datagram, len + DATAGRAM_HEADER);
}

bool oldStatus = false;

void BleUartClient::loop(void)
{
	if (bleuart.available())
	{
		rxLen = bleuart.read(rxData, sizeof(rxData));
		handleData(rxData, rxLen);
	}
	if (notifEnabled && !oldStatus)
	{
		oldStatus = true;

		if (username.length() != 0)
		{
			Datagram nameDatagram;
			nameDatagram.type = 'u';
			int msgLen = sprintf((char *)nameDatagram.message, "00u|%s", username.c_str());
			receive(nameDatagram, msgLen + DATAGRAM_HEADER);
		}
		if (history)
		{
			history->replay(&ble_client);
		}
	}
	if (!notifEnabled && oldStatus)
	{
		oldStatus = false;
	}
}

void BleUartClient::startServer(void (*callback)(BleUartClient *))
{
	connectCallback = callback;

	init();
}

void BleUartClient::init()
{
	// Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
	Bluefruit.configPrphConn(247, BLE_GAP_EVENT_LENGTH_MIN, BLE_GATTS_HVN_TX_QUEUE_SIZE_DEFAULT, BLE_GATTC_WRITE_CMD_TX_QUEUE_SIZE_DEFAULT);
	Bluefruit.begin();
	// Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
	Bluefruit.setTxPower(4);

	// Using nRF52 unique ID
	sprintf(apName, "DR-%s", macaddr);
#ifdef DEBUG_OUT
	Serial.printf("Device name: %s\n", apName);
#endif

	// Initialize BLE and set output power
	Bluefruit.setName(apName);

	Bluefruit.autoConnLed(false);

	Bluefruit.Periph.setConnectCallback(connect_callback);
	Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

	// To be consistent OTA DFU should be added first if it exists
	bledfu.begin();

	// Configure and Start BLE Uart Service
	bleuart.begin();

	bleuart.setNotifyCallback(uartNotifyCallback);

	// Advertising packet
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();
	Bluefruit.Advertising.addName();

	/* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds 
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
	Bluefruit.Advertising.start(0);				// 0 = Don't stop advertising after n seconds
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
	(void)conn_handle;
#ifdef DEBUG_OUT
	Serial.println("BLE client connected");
#endif
	deviceConnected = true;
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void)conn_handle;
	(void)reason;
#ifdef DEBUG_OUT
	Serial.println("BLE client disconnected");
#endif
	deviceConnected = false;
	notifEnabled = false;
}

/** 
 * Callback invoked when client enables notifications.
 * Data should not be sent before notifications are enabled
 * @param conn_handle connection where this event happens
 * @param status notification status
 */
void uartNotifyCallback(uint16_t connHandle, boolean status)
{
	if(status)
	{
#ifdef DEBUG_OUT
		Serial.println("Notifications enabled");
#endif
		if (connectCallback)
		{
			if (!notifEnabled)
			{
				connectCallback(&ble_client);
				notifEnabled = true;
			}
			else
			{
				if (history)
				{
					history->replay(&ble_client);
				}
			}
		}
	}
	else
	{
#ifdef DEBUG_OUT
		Serial.println("Notifications disabled");
#endif
		notifEnabled = false;
	}
}
#endif
