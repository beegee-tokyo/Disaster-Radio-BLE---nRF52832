#include "settings.h"

Preferences preferences;

String username = "";
bool useBLE = true;

void getSettings(void)
{

  // // One time for Wrover only
  // nvs_flash_init();
  // nvs_flash_erase();

  if (!preferences.begin("dr", false))
  {
    Serial.println("Error opening preferences");
    nvs_flash_init();
    return;
  }

  username = preferences.getString("un", "");

  if (username.isEmpty())
  {
    Serial.println("No name saved");
  }

  Serial.printf("Got username %s\n", username.c_str());

  useBLE = preferences.getBool("ble", true);

  Serial.printf("Got IF setting %s\n", useBLE ? "BLE" : "WiFi");

  preferences.end();
  return;
}

void saveUsername(String newUserName)
{
  if (!preferences.begin("dr", false))
  {
    Serial.println("Error opening preferences");
    return;
  }

  if (newUserName.isEmpty())
  {
    preferences.remove("un");
    Serial.println("Deleted saved username");
  }
  else
  {
    preferences.putString("un", newUserName);
    Serial.printf("Saved username '%s'\n", preferences.getString("un", "ERROR SAVING").c_str());
  }
  preferences.end();
}

void saveUI(bool useBLE)
{
  if (!preferences.begin("dr", false))
  {
    Serial.println("Error opening preferences");
    return;
  }

  preferences.putBool("ble", useBLE);
  preferences.end();
}