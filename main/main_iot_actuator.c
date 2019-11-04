/*	ESP32 IoT Light Actuator for Amazon cloud

	- Connect to WiFi
	- Connect to AWS IoT cloud
	- Get state from Shadow
	- Power on/off connected lamp
*/
//-----------------------------------------------------------------------------
// FreeRTOS
#include "../build/config/sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
// Espressif
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
// Project
#include "blink.h"
#include "wifi.h"
#include "thing.h"
//-----------------------------------------------------------------------------
// Define TAGs for log messages
#define	TAG_MAIN	"APP"
//-----------------------------------------------------------------------------
// Pin connected to MOSFET key that operates relay
#define LAMP_PIN			GPIO_NUM_13
//-----------------------------------------------------------------------------
/* FreeRTOS event group to to check signals */
static EventGroupHandle_t events_group;
const int IP_UP_BIT = BIT0;	// Bit to check IP link readiness
const int STATE_BIT = BIT1; // Bit is set if state was changed
////-----------------------------------------------------------------------------
//void lamp_actuate_callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext)
//{
//    //IOT_UNUSED(pJsonString);
//    //IOT_UNUSED(JsonStringDataLen);
//    ESP_LOGI(TAG_AWS, "RECEIVED %s", pJsonString);
//
//    ESP_LOGI(TAG_AWS, "callback fired");
//    if (pContext->type != SHADOW_JSON_BOOL)
//    {
//    	ESP_LOGI(TAG_AWS, "WRONG TYPE");
//    	return;
//    }
//    if (xEventGroupGetBits(events_group) & STATE_BIT)
//    {
//    	ESP_LOGI(TAG_AWS, "Previous update of \"lampOn\" in progress");
//    	return;
//    }
//
//    if(pContext != NULL)
//    {
//        ESP_LOGI(TAG_AWS, "Delta \"lampOn\" received: %d (type %d, len %d)", *(bool *)(pContext->pData), pContext->type, JsonStringDataLen);
//        if (*(bool *)(pContext->pData))
//        {
//        	gpio_set_level(LED_PIN, 1);
//        	LampIsOn = true;
//        }
//        else
//        {
//        	gpio_set_level(LED_PIN, 0);
//        	LampIsOn = false;
//        }
//        xEventGroupSetBits(events_group, STATE_BIT);
//    }
//}
//-----------------------------------------------------------------------------
void app_main(void)
{
	ESP_LOGI(TAG_MAIN, "Light Actuator v1.0 STARTED");
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_LOGI(TAG_MAIN, "Flash initialized");
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_LOGI(TAG_MAIN, "Event loop created");
	events_group = xEventGroupCreate();

//	gpio_set_direction(LAMP_PIN, GPIO_MODE_OUTPUT);

	blink_start();

	wifi_start(events_group, IP_UP_BIT);

	aws_start(events_group, IP_UP_BIT);
}
