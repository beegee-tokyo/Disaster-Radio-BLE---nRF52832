// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef USE_SX1262

#include "LoRa1262.h"

#ifdef NRF52
// Singleton for SPI connection to the LoRa chip
SPIClass SPI_LORA(NRF_SPIM1, MISO, SCK, MOSI);
#endif

#if (ESP8266 || ESP32)
#define ISR_PREFIX ICACHE_RAM_ATTR
#else
#define ISR_PREFIX
#endif

/** HW configuration structure for the LoRa library */
hw_config hwConfig;
/** Task to handle time synchronization */
TaskHandle_t loraTaskHandle = NULL;

/** Buffer for outgoing data */
uint8_t txBuffer[256];
/** Size of package */
uint16_t txSize = 0;
/** Flag if TX is active */
bool txActive = false;
/** Flag if TX is finished */
bool txDone = false;
/** Flag if package has been received */
bool hasPackage = false;
/** Buffer for received data */
uint8_t rxBuffer[256];
/** Index of rxBuffer used when reading byte wise */
int _rxPacketIndex = 0;
/** Last RSSI */
int16_t _rxRssi = 0;
/** Last SNR */
int8_t _rxSnr = 0;
/** Flag for LoRa task */
bool pckgAlert = false;
/** Size of received package */
uint16_t _rxSize = 0;

/** LoRa callback events */
static RadioEvents_t RadioEvents;
/** Layer 1 callback */
void (*_onReceive2)(int);

/** Radio settings */
int8_t power = TX_OUTPUT_POWER;
uint32_t fdev = 0;
uint32_t bandwidth = LORA_BANDWIDTH;
uint32_t datarate = LORA_SPREADING_FACTOR;
uint8_t coderate = LORA_CODINGRATE;
uint16_t preambleLen = LORA_PREAMBLE_LENGTH;
bool fixLen = LORA_FIX_LENGTH_PAYLOAD_ON;
uint8_t payloadLen = 0;
bool crcOn = true;
bool freqHopOn = 0;
uint8_t hopPeriod = 0;
bool iqInverted = LORA_IQ_INVERSION_ON;
uint16_t symbTimeout = LORA_SYMBOL_TIMEOUT;
uint32_t bandwidthAfc = 0;

bool beginDone = false;

LoRaClass::LoRaClass() : _spiSettings(100000, MSBFIRST, SPI_MODE0),
						 _ss(LORA_NSS), _reset(LORA_RESET), _dio0(LORA_DIO1),
						 _frequency(RF_FREQUENCY),
						 _packetIndex(0),
						 _implicitHeaderMode(0),
						 _onReceive(NULL)
{
	// overide Stream timeout value
	setTimeout(0);
	_onReceive2 = NULL;
	_rxPacketIndex = 0;
}

/**
 * Task to handle the LoRa events
 */
void loraTask(void *pvParameters)
{
	while (1)
	{
		Radio.IrqProcess();

		if (pckgAlert)
		{
			pckgAlert = false;

			_rxPacketIndex = 0;

			if (_onReceive2)
			{
				_onReceive2(_rxSize);
			}
		}
		delay(100);
	}
}

/**
 * Callback after a LoRa package was received
 * @param payload
 * 			Pointer to the received data
 * @param size
 * 			Length of the received package
 * @param rssi
 * 			Signal strength while the package was received
 * @param snr
 * 			Signal to noise ratio while the package was received
 */
void OnRxDone(uint8_t *rxPayload, uint16_t rxSize, int16_t rxRssi, int8_t rxSnr)
{
	// Secure buffer before restart listening
	if (rxSize < 256)
	{
		memcpy(rxBuffer, rxPayload, rxSize + 1);
	}
	else
	{
		memcpy(rxBuffer, rxPayload, rxSize);
	}

	_rxRssi = rxRssi;
	_rxSnr = rxSnr;
	_rxSize = rxSize;

	hasPackage = true;
	pckgAlert = true;
}

/**
 * Callback after a package was successfully sent
 */
void OnTxDone(void)
{
	txActive = false;
	txDone = true;
	// Restart listening
	Radio.Standby();
	Radio.Rx(0);
}

/**
 * Callback after a package was successfully sent
 */
void OnTxError(void)
{
	txActive = false;
	txDone = true;
	// Restart listening
	Radio.Standby();
	Radio.Rx(0);
}

void LoRaClass::setRadioConfig(void)
{
	// Put LoRa into standby
	Radio.Standby();

	// Set transmit configuration
	Radio.SetTxConfig(MODEM_LORA, power, fdev, bandwidth,
					  datarate, coderate,
					  preambleLen, fixLen,
					  crcOn, freqHopOn, hopPeriod, iqInverted, TX_TIMEOUT_VALUE);

	// Set receive configuration
	Radio.SetRxConfig(MODEM_LORA, bandwidth, datarate,
					  coderate, bandwidthAfc, preambleLen,
					  symbTimeout, fixLen, payloadLen,
					  crcOn, freqHopOn, hopPeriod, iqInverted, true);
}

int LoRaClass::begin(long frequency)
{
	#ifdef ISP4520
	if (lora_isp4520_init(SX1262) != 0)
	{
		Serial.println("Error in hardware init");
		return 0;
	}
#else
	// Define the HW configuration between MCU and SX126x
	hwConfig.CHIP_TYPE = SX1262_CHIP;	  // eByte E22 module with an SX1262
	hwConfig.PIN_LORA_RESET = LORA_RESET;  // LORA RESET
	hwConfig.PIN_LORA_NSS = LORA_NSS;	  // LORA SPI CS
	hwConfig.PIN_LORA_SCLK = LORA_SCLK;	// LORA SPI CLK
	hwConfig.PIN_LORA_MISO = LORA_MISO;	// LORA SPI MISO
	hwConfig.PIN_LORA_DIO_1 = LORA_DIO1;   // LORA DIO_1
	hwConfig.PIN_LORA_BUSY = LORA_BUSY;	// LORA SPI BUSY
	hwConfig.PIN_LORA_MOSI = LORA_MOSI;	// LORA SPI MOSI
	hwConfig.RADIO_TXEN = LORA_RADIO_TXEN; // LORA ANTENNA TX ENABLE
	hwConfig.RADIO_RXEN = LORA_RADIO_RXEN; // LORA ANTENNA RX ENABLE
	hwConfig.USE_DIO2_ANT_SWITCH = true;   // Example uses an eByte E22 module which uses RXEN and TXEN pins as antenna control
	hwConfig.USE_DIO3_TCXO = true;		   // Example uses an eByte E22 module which uses DIO3 to control oscillator voltage
	hwConfig.USE_DIO3_ANT_SWITCH = false;  // Only Insight ISP4520 module uses DIO3 as antenna control

	if (lora_hardware_init(hwConfig) != 0)
	{
		Serial.println("Error in hardware init");
		return 0;
	}
#endif
	// Initialize the callbacks
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxError;

	Radio.Init(&RadioEvents);

	// Put LoRa into standby
	Radio.Standby();

	// Set Frequency
	Radio.SetChannel(frequency);

	setRadioConfig();

	// Start waiting for data package
	Radio.Standby();

	SX126xSetDioIrqParams(IRQ_RADIO_ALL,
						  IRQ_RADIO_ALL,
						  IRQ_RADIO_NONE, IRQ_RADIO_NONE);
	Radio.Rx(0);

	Serial.println("Starting Lora Handler Task");
	if (!xTaskCreate(loraTask, "Lora", 2048, NULL, 1, &loraTaskHandle))
	{
		Serial.println("Starting Lora Handler Task failed");
		return 0;
	}
	else
	{
		Serial.println("Starting Mesh Sync Task success");
	}
	beginDone = true;

	return 1;
}

void LoRaClass::end()
{
	// put in sleep mode
	Radio.Sleep();
}

int LoRaClass::beginPacket(int implicitHeader)
{
	txSize = 0;
	txActive = true;
	return 1;
}

int LoRaClass::endPacket(bool async)
{
	Radio.Standby();
	Radio.Send((uint8_t *)&txBuffer, txSize);
	txDone = false;

	while (!txDone)
	{
		delay(10);
	}

	memset(txBuffer, 0, 256);
	txSize = 0;
	return 1;
}

bool LoRaClass::isTransmitting()
{
	return txActive;
}

int LoRaClass::parsePacket(int size)
{
	return 0;
}

int LoRaClass::packetRssi()
{
	return _rxRssi;
}

float LoRaClass::packetSnr()
{
	return _rxSnr;
}

long LoRaClass::packetFrequencyError()
{
	return 0;
}

size_t LoRaClass::write(uint8_t byte)
{
	txBuffer[txSize] = byte;
	txSize++;
	return 1;
}

size_t LoRaClass::write(const uint8_t *buffer, size_t size)
{
	if (size < 256)
	{
		memcpy(txBuffer, buffer, size);
		txSize = size;
		return size;
	}
	return 0;
}

int LoRaClass::available()
{
	return _rxPacketIndex - _rxSize;
}

int LoRaClass::read()
{
	if (!available())
	{
		return -1;
	}

	_rxPacketIndex++;

	return rxBuffer[_rxPacketIndex - 1];
}

int LoRaClass::peek()
{
	if (!available())
	{
		return -1;
	}

	return rxBuffer[_packetIndex - 1];
}

void LoRaClass::flush()
{
}

void LoRaClass::onReceive(void (*callback)(int))
{
	_onReceive = callback;
	_onReceive2 = callback;
}

void LoRaClass::receive(int size)
{
	Radio.Standby();

	SX126xSetDioIrqParams(IRQ_RADIO_ALL,
						  IRQ_RADIO_ALL,
						  IRQ_RADIO_NONE, IRQ_RADIO_NONE);
	Radio.Rx(0);
}

void LoRaClass::idle()
{
	Radio.Standby();
}

void LoRaClass::sleep()
{
	Radio.Sleep();
}

void LoRaClass::setTxPower(int level, int outputPin)
{
	// SX127x has max power 17, SX126x has max power 22, lets translate it!
	level = level * 22 / 17;
	
	if ((level < 0) || (level > 22))
	{
		power = TX_OUTPUT_POWER;
	}
	else
	{
		power = level;
	}

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::setFrequency(long frequency)
{
	Radio.SetChannel(frequency);
}

int LoRaClass::getSpreadingFactor()
{
	return datarate;
}

void LoRaClass::setSpreadingFactor(int sf)
{
	if (sf < 6)
	{
		sf = 6;
	}
	else if (sf > 12)
	{
		sf = 12;
	}

	datarate = sf;

	if (beginDone)
	{
		setRadioConfig();
	}
}

long LoRaClass::getSignalBandwidth()
{
	return bandwidth;
}

void LoRaClass::setSignalBandwidth(long sbw)
{
	int bw;

	if (sbw <= 7.8E3)
	{
		bw = 0;
	}
	else if (sbw <= 10.4E3)
	{
		bw = 1;
	}
	else if (sbw <= 15.6E3)
	{
		bw = 2;
	}
	else if (sbw <= 20.8E3)
	{
		bw = 3;
	}
	else if (sbw <= 31.25E3)
	{
		bw = 4;
	}
	else if (sbw <= 41.7E3)
	{
		bw = 5;
	}
	else if (sbw <= 62.5E3)
	{
		bw = 6;
	}
	else if (sbw <= 125E3)
	{
		bw = 7;
	}
	else if (sbw <= 250E3)
	{
		bw = 8;
	}
	else /*if (sbw <= 250E3)*/
	{
		bw = 9;
	}

	bandwidth = bw;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::setLdoFlag()
{
	return;
}

void LoRaClass::setCodingRate4(int denominator)
{
	if (denominator < 5)
	{
		denominator = 5;
	}
	else if (denominator > 8)
	{
		denominator = 8;
	}

	coderate = denominator - 4;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::setPreambleLength(long length)
{
	preambleLen = length;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::setSyncWord(int sw)
{
	uint8_t syncWord[8];
	syncWord[0] = (uint8_t)sw;
	syncWord[1] = (uint8_t)(sw >> 8);
	syncWord[2] = 0x00;
	syncWord[3] = 0x00;
	syncWord[4] = 0x00;
	syncWord[5] = 0x00;
	syncWord[6] = 0x00;
	syncWord[7] = 0x00;
	SX126xSetSyncWord(syncWord);
}

void LoRaClass::enableCrc()
{
	crcOn = true;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::disableCrc()
{
	crcOn = false;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::enableInvertIQ()
{
	iqInverted = false;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::disableInvertIQ()
{
	iqInverted = false;

	if (beginDone)
	{
		setRadioConfig();
	}
}

void LoRaClass::setOCP(uint8_t mA)
{
	return;
}

byte LoRaClass::random()
{
	return _rxRssi;
}

void LoRaClass::setPins(int ss, int reset, int dio0)
{
	return;
}

void LoRaClass::setSPI(SPIClass &spi)
{
	return;
}

void LoRaClass::setSPIFrequency(uint32_t frequency)
{
	return;
}

void LoRaClass::dumpRegisters(Stream &out)
{
	return;
}

void LoRaClass::explicitHeaderMode()
{
	return;
}

void LoRaClass::implicitHeaderMode()
{
	return;
}

void LoRaClass::handleDio0Rise()
{
	return;
}

uint8_t LoRaClass::readRegister(uint8_t address)
{
	return singleTransfer(address & 0x7f, 0x00);
}

void LoRaClass::writeRegister(uint8_t address, uint8_t value)
{
	singleTransfer(address | 0x80, value);
}

uint8_t LoRaClass::singleTransfer(uint8_t address, uint8_t value)
{
	uint8_t response;

	digitalWrite(_ss, LOW);

	_spi->beginTransaction(_spiSettings);
	_spi->transfer(address);
	response = _spi->transfer(value);
	_spi->endTransaction();

	digitalWrite(_ss, HIGH);

	return response;
}

ISR_PREFIX void LoRaClass::onDio0Rise()
{
	LoRa.handleDio0Rise();
}

LoRaClass LoRa;
#endif
