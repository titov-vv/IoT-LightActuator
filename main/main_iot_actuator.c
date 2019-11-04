/*	ESP32 IoT Light Actuator for Amazon cloud

	- Connect to WiFi
	- Connect to AWS IoT cloud
	- Get state from Shadow
	- Power on/off connected lamp
*/
//https://github.com/espressif/esp-aws-iot/blob/master/examples/thing_shadow/main/thing_shadow_sample.c
//https://github.com/espressif/esp-aws-iot/blob/master/examples/subscribe_publish/main/subscribe_publish_sample.c
#include <stdio.h>

#include "../build/config/sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

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
//static bool	LampIsOn = false;
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
////-----------------------------------------------------------------------------
//void aws_iot_task(void *arg)
//{
//    IoT_Error_t rc = FAILURE;
//    AWS_IoT_Client aws_client;
//    ShadowInitParameters_t shadowParams = ShadowInitParametersDefault;
//    ShadowConnectParameters_t connectParams = ShadowConnectParametersDefault;
//
//    bool lampOn = false;
//    jsonStruct_t lamp_actuator;
//    lamp_actuator.cb = lamp_actuate_callback;
//    lamp_actuator.pData = &lampOn;
//    lamp_actuator.pKey = "lampOn";
//    lamp_actuator.type = SHADOW_JSON_BOOL;
//    lamp_actuator.dataLength = sizeof(bool);
//
//    ESP_LOGI(TAG_AWS, "Shadow init started");
//    shadowParams.enableAutoReconnect = false;
//    shadowParams.pHost = AWS_host;
//    shadowParams.port = AWS_port;
//    shadowParams.pRootCA  = aws_root_ca_pem;
//    shadowParams.pClientCRT  = certificate_pem_crt;
//    shadowParams.pClientKey  = private_pem_key;
//    shadowParams.disconnectHandler = aws_disconnect_handler;
//
//    rc = aws_iot_shadow_init(&aws_client, &shadowParams);
//    if (rc != SUCCESS)
//    {
//        ESP_LOGE(TAG_AWS, "Shadow init failure: %d ", rc);
//        vTaskDelete(NULL);
//        return;
//    }
//    ESP_LOGI(TAG_AWS, "Shadow init success");
//
//    ESP_LOGI(TAG_AWS, "Wait for IP link");
//    xEventGroupWaitBits(events_group, IP_UP_BIT, false, true, portMAX_DELAY);
//    ESP_LOGI(TAG_AWS, "IP link is up");
//
//    ESP_LOGI(TAG_AWS, "Shadow connect started");
//    connectParams.pMyThingName = AWS_CLIENTID;
//    connectParams.pMqttClientId = AWS_CLIENTID;
//    connectParams.mqttClientIdLen = (uint16_t) strlen(AWS_CLIENTID);
//    do
//    {
//        rc = aws_iot_shadow_connect(&aws_client, &connectParams);
//        if(rc != SUCCESS)
//        {
//            ESP_LOGE(TAG_AWS, "Shadow connect error: %d", rc);
//            vTaskDelay(5000 / portTICK_RATE_MS);
//        }
//    }
//    while(rc != SUCCESS);
//    ESP_LOGI(TAG_AWS, "Shadow connected");
//
//    rc = aws_iot_mqtt_autoreconnect_set_status(&aws_client, true);
//    if(rc == SUCCESS)
//    	ESP_LOGI(TAG_AWS, "Shadow auto-reconnect enabled");
//    else
//    	ESP_LOGE(TAG_AWS, "Shadow auto-reconnect setup failure: %d ", rc);
//
//    rc = aws_iot_shadow_register_delta(&aws_client, &lamp_actuator);
//    if(rc == SUCCESS)
//        ESP_LOGI(TAG_AWS, "Shadow Delta callback registered");
//    else
//     	ESP_LOGE(TAG_AWS, "Shadow Delta callback failed: %d ", rc);
//
//
//    while(1)
//    {
//    	rc = aws_iot_shadow_yield(&aws_client, 100);
//        if (rc != SUCCESS)
//        	continue;
//
//        if (xEventGroupGetBits(events_group) & STATE_BIT)
//        {
//
////    Update shadow state
////        	{
////        	  "state": {
////        	    "reported": {
////        	      "lampOn": false
////        	    }
////        	  }
////        	}
//        	ESP_LOGI(TAG_AWS, "Report state");
//        	root = cJSON_CreateObject();
//        	state = cJSON_CreateObject();
//        	cJSON_AddItemToObject(root, "state", state);
//        	reported = cJSON_CreateObject();
//        	cJSON_AddItemToObject(state, "reported", reported);
//        	cJSON_AddBoolToObject(reported, "lampOn", LampIsOn);
//
//        	if (!cJSON_PrintPreallocated(root, JSON_buffer, MAX_JSON_SIZE, 0 /*  not formatted */))
//        	{
//        		ESP_LOGW(TAG_AWS, "JSON buffer too small");
//        	    JSON_buffer[0] = 0;
//        	}
//        	cJSON_Delete(root);
//        	ESP_LOGI(TAG_AWS, "JSON Thing reported state: %s", JSON_buffer);
//
//        	rc = aws_iot_shadow_update(&aws_client, AWS_CLIENTID, JSON_buffer, shadow_update_callback, NULL, 4, true);
//
//        	xEventGroupClearBits(events_group, STATE_BIT);
//        }
//
//        vTaskDelay(1000 / portTICK_RATE_MS);
//    }
//
//	vTaskDelete(NULL);
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
