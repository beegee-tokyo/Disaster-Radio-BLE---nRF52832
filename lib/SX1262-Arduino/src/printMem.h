#include <Arduino.h>
#ifdef ESP32
#include <esp_system.h>

void printMem(void)
{
	Serial.println("\n\n[HEA] ##################################");
	Serial.printf("[HEA] Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("[HEA] SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
	Serial.println("[HEA] ##################################\n\n");
}
#else
void printMem(void)
{
	Serial.println("\n\n[HEA] ##################################");
	Serial.printf("[HEA] Internal Total heap %d, internal Free Heap %d\n", dbgHeapTotal(), dbgHeapFree());
	Serial.println("[HEA] ##################################\n\n");
}
#endif