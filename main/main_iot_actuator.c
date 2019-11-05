/*	ESP32 IoT Light Actuator for Amazon cloud

	- Connect to WiFi
	- Connect to AWS IoT cloud
	- Get state from Shadow
	- Power on/off connected lamp
*/
//-----------------------------------------------------------------------------
#include "../build/config/sdkconfig.h"
// Espressif
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
// Project
#include "main.h"
#include "blink.h"
#include "wifi.h"
#include "thing.h"
//-----------------------------------------------------------------------------
// FreeRTOS event group to to synchronize between tasks
EventGroupHandle_t events_group;
//-----------------------------------------------------------------------------
void app_main(void)
{
	ESP_LOGI(TAG_MAIN, "Light Actuator v1.0 STARTED");
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_LOGI(TAG_MAIN, "Flash initialized");
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_LOGI(TAG_MAIN, "Event loop created");
	events_group = xEventGroupCreate();

	blink_start();

	wifi_start();

	aws_start();
}
