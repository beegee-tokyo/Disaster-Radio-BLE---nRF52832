#ifndef NRF52
#ifndef OLEDCLIENT_H
#define OLEDCLIENT_H

#include <list>

#include "../DisasterClient.h"
#include "../DisasterServer.h"
#include "../configDR.h"

#ifdef HAS_DISPLAY
#include <SH1106Wire.h>
#else
#include "SSD1306Wire.h"
#endif
class OLEDClient : public DisasterClient
{
#ifdef HAS_DISPLAY
	SH1106Wire *display;
#else
  SSD1306Wire *display;
#endif
	
  std::list<String> buffer;

  int left;
  int top;
  int width = OLED_WIDTH;
  int height = OLED_HEIGHT;

  bool needs_display = false;

public:
#ifdef HAS_DISPLAY
	OLEDClient(SH1106Wire *d, int l = 0, int t = 0)
		: display{d}, left{l}, top{t} {};
#else
  OLEDClient(SSD1306Wire *d, int l = 0, int t = 0)
      : display{d}, left{l}, top{t} {};
#endif
  void loop();
  void receive(struct Datagram datagram, size_t len);
};

#endif
#endif
