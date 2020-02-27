#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#ifndef NRF52
#include <Preferences.h>
#include <nvs_flash.h>
#endif
void getSettings(void);
void saveUsername(String newUserName);
void saveUI(bool useBLE);

extern String username;
extern bool useBLE;
#endif