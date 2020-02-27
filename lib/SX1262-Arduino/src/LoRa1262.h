// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef USE_SX1262

#ifndef LORA1262_H
#define LORA1262_H

#include <Arduino.h>
#include <SPI.h>
#include "SX126x-Arduino.h"
// #include <config.h>

#ifdef IS_WROVER
// ESP32 Wrover - SX126x pin configuration (Gateway Box)
/** LORA RESET */
#define LORA_RESET 4
/** LORA DIO_1 */
#define LORA_DIO1 21
/** LORA SPI BUSY */
#define LORA_BUSY 22
/** LORA SPI CS */
#define LORA_NSS 5
/** LORA SPI CLK */
#define LORA_SCLK 18
/** LORA SPI MISO */
#define LORA_MISO 19
/** LORA SPI MOSI */
#define LORA_MOSI 23
/** LORA ANTENNA TX ENABLE */
#define LORA_RADIO_TXEN 26
/** LORA ANTENNA RX ENABLE */
#define LORA_RADIO_RXEN 27
#endif
#ifdef IS_FEATHER
/** LORA RESET */
#define LORA_RESET 32
/** LORA DIO_1 */
#define LORA_DIO1 14
/** LORA SPI BUSY */
#define LORA_BUSY 27
/** LORA SPI CS */
#define LORA_NSS 33
/** LORA SPI CLK */
#define LORA_SCLK 5
/** LORA SPI MISO */
#define LORA_MISO 19
/** LORA SPI MOSI */
#define LORA_MOSI 18
/** LORA ANTENNA TX ENABLE */
#define LORA_RADIO_TXEN -1
/** LORA ANTENNA RX ENABLE */
#define LORA_RADIO_RXEN -1
#endif
#ifdef NRF52
/** LORA RESET */
#define LORA_RESET 30
/** LORA DIO_1 */
#define LORA_DIO1 27
/** LORA SPI BUSY */
#define LORA_BUSY 7
/** LORA SPI CS */
#define LORA_NSS 11
/** LORA SPI CLK */
#define LORA_SCLK SCK
/** LORA SPI MISO */
#define LORA_MISO MISO
/** LORA SPI MOSI */
#define LORA_MOSI MOSI
/** LORA ANTENNA TX ENABLE */
#define LORA_RADIO_TXEN -1
/** LORA ANTENNA RX ENABLE */
#define LORA_RADIO_RXEN -1
// // Singleton for SPI connection to the LoRa chip
// SPIClass SPI_LORA(NRF_SPIM1, MISO, SCK, MOSI);
#endif

// LoRa definitions
#define RF_FREQUENCY 915000000  // Hz
#define TX_OUTPUT_POWER 22		// dBm
#define LORA_BANDWIDTH 0		// [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 9 // [SF7..SF12]
#define LORA_CODINGRATE 1		// [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 5000
#define TX_TIMEOUT_VALUE 5000

class LoRaClass : public Stream {
public:
  LoRaClass();

  int begin(long frequency);
  void end();

  int beginPacket(int implicitHeader = false);
  int endPacket(bool async = false);

  int parsePacket(int size = 0);
  int packetRssi();
  float packetSnr();
  long packetFrequencyError();

  // from Print
  virtual size_t write(uint8_t byte);
  virtual size_t write(const uint8_t *buffer, size_t size);

  // from Stream
  virtual int available();
  virtual int read();
  virtual int peek();
  virtual void flush();

  void onReceive(void(*callback)(int));

  void receive(int size = 0);

  void idle();
  void sleep();

  void setTxPower(int level, int outputPin = 50);
  void setFrequency(long frequency);
  void setSpreadingFactor(int sf);
  void setSignalBandwidth(long sbw);
  void setCodingRate4(int denominator);
  void setPreambleLength(long length);
  void setSyncWord(int sw);
  void enableCrc();
  void disableCrc();
  void enableInvertIQ();
  void disableInvertIQ();
  
  void setOCP(uint8_t mA); // Over Current Protection control

  // deprecated
  void crc() { enableCrc(); }
  void noCrc() { disableCrc(); }

  byte random();

  void setPins(int ss = LORA_NSS, int reset = LORA_RESET, int dio0 = LORA_DIO1);
  void setSPI(SPIClass& spi);
  void setSPIFrequency(uint32_t frequency);

  void dumpRegisters(Stream& out);

private:
	void setRadioConfig(void);

	void explicitHeaderMode();
	void implicitHeaderMode();

	void handleDio0Rise();
	bool isTransmitting();

	int getSpreadingFactor();
	long getSignalBandwidth();

	void setLdoFlag();

	uint8_t readRegister(uint8_t address);
	void writeRegister(uint8_t address, uint8_t value);
	uint8_t singleTransfer(uint8_t address, uint8_t value);

	static void onDio0Rise();

	SPISettings _spiSettings;
	SPIClass *_spi;
	int _ss;
	int _reset;
	int _dio0;
	long _frequency;
	int _packetIndex;
	int _implicitHeaderMode;
	void (*_onReceive)(int);

private:
//   SPISettings _spiSettings;
//   SPIClass* _spi;
//   int _ss;
//   int _reset;
//   int _dio0;
//   long _frequency;
//   int _packetIndex;
//   int _implicitHeaderMode;
//   void (*_onReceive)(int);
};

extern LoRaClass LoRa;

#endif
#endif