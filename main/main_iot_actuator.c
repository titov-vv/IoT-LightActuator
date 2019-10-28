/*	ESP32 IoT Light Actuator for Amazon cloud

	- Connect to WiFi
	- Connect to AWS IoT cloud
	- Get state from Shadow
	- Power on/off connected lamp
*/
#include <stdio.h>
#include <string.h>
#include "../build/config/sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"

#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#include "aws_config.h"
// Here is a private file with WiFi connection details
// It contains defines for WIFI_SSID and WIFI_PASSWORD
#include "wifi_credentials.h"
//-----------------------------------------------------------------------------
// Define TAGs for log messages
#define	TAG_MAIN	"APP"
#define TAG_WIFI	"WIFI"
#define TAG_AWS		"AWS"
//-----------------------------------------------------------------------------
#define LED_PIN				GPIO_NUM_2
// Pin connected to MOSFET key that operates relay
#define LAMP_PIN			GPIO_NUM_13
//-----------------------------------------------------------------------------
char AWS_host[255] = AWS_HOST;
uint32_t AWS_port = AWS_PORT;
//-----------------------------------------------------------------------------
static int wifi_retry = 0;
/* FreeRTOS event group to to check signals */
static EventGroupHandle_t events_group;
const int IP_UP_BIT = BIT0;	// Bit to check IP link readiness
//-----------------------------------------------------------------------------
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
		case WIFI_EVENT_STA_START:
			ESP_LOGI(TAG_WIFI, "Connecting...");
			esp_wifi_connect();
			xEventGroupClearBits(events_group, IP_UP_BIT);
			break;
		case WIFI_EVENT_STA_CONNECTED:
			wifi_retry = 0;
			ESP_LOGI(TAG_WIFI, "Connected");
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			wifi_retry++;
			ESP_LOGI(TAG_WIFI, "Reconnection attempt %d", wifi_retry);
			esp_wifi_connect();
			xEventGroupClearBits(events_group, IP_UP_BIT);
			break;
		}
	}

	if (event_base == IP_EVENT)
	{
		switch(event_id)
		{
		case IP_EVENT_STA_GOT_IP:
			xEventGroupSetBits(events_group, IP_UP_BIT);
			ESP_LOGI(TAG_WIFI, "Received IP: %s", ip4addr_ntoa(&((ip_event_got_ip_t*)event_data)->ip_info.ip));
			break;
		}
	}
}
//-----------------------------------------------------------------------------
void wifi_start(void)
{
	ESP_LOGI(TAG_WIFI, "Initialization started");
	tcpip_adapter_init();
	wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
// TODO
// ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
	ESP_LOGI(TAG_WIFI, "Initialization completed");

	ESP_LOGI(TAG_WIFI, "Connect to '%s'", WIFI_SSID);
	wifi_config_t wifi_cfg = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD }};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG_WIFI, "Connection start...");
}
//-----------------------------------------------------------------------------
void lamp_actuate_callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);

    if(pContext != NULL)
    {
        ESP_LOGI(TAG_AWS, "Delta - lampOn state changed to %d", *(bool *) (pContext->pData));
        if (*(bool *)(pContext->pData))
        	gpio_set_level(LED_PIN, 1);
        else
        	gpio_set_level(LED_PIN, 1);
    }
}
//-----------------------------------------------------------------------------
void aws_disconnect_handler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(TAG_AWS, "MQTT Disconnect");
}
//-----------------------------------------------------------------------------
void aws_iot_task(void *arg)
{
    IoT_Error_t rc = FAILURE;
    AWS_IoT_Client aws_client;
    ShadowInitParameters_t shadowParams = ShadowInitParametersDefault;
    ShadowConnectParameters_t connectParams = ShadowConnectParametersDefault;

    bool lampOn = false;
    jsonStruct_t lamp_actuator;
    lamp_actuator.cb = lamp_actuate_callback;
    lamp_actuator.pData = &lampOn;
    lamp_actuator.pKey = "lampOn";
    lamp_actuator.type = SHADOW_JSON_BOOL;
    lamp_actuator.dataLength = sizeof(bool);

    ESP_LOGI(TAG_AWS, "Shadow init started");
    shadowParams.enableAutoReconnect = false;
    shadowParams.pHost = AWS_host;
    shadowParams.port = AWS_port;
    shadowParams.pRootCA  = aws_root_ca_pem;
    shadowParams.pClientCRT  = certificate_pem_crt;
    shadowParams.pClientKey  = private_pem_key;
    shadowParams.disconnectHandler = aws_disconnect_handler;

    rc = aws_iot_shadow_init(&aws_client, &shadowParams);
    if (rc != SUCCESS)
    {
        ESP_LOGE(TAG_AWS, "Shadow init failure: %d ", rc);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG_AWS, "Shadow init success");

    ESP_LOGI(TAG_AWS, "Wait for IP link");
    xEventGroupWaitBits(events_group, IP_UP_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG_AWS, "IP link is up");

    ESP_LOGI(TAG_AWS, "Shadow connect started");
    connectParams.pMyThingName = AWS_CLIENTID;
    connectParams.pMqttClientId = AWS_CLIENTID;
    connectParams.mqttClientIdLen = (uint16_t) strlen(AWS_CLIENTID);
    do
    {
        rc = aws_iot_shadow_connect(&aws_client, &connectParams);
        if(rc != SUCCESS)
        {
            ESP_LOGE(TAG_AWS, "Shadow connect error: %d", rc);
            vTaskDelay(5000 / portTICK_RATE_MS);
        }
    }
    while(rc != SUCCESS);
    ESP_LOGI(TAG_AWS, "Shadow connected");

    rc = aws_iot_mqtt_autoreconnect_set_status(&aws_client, true);
    if(rc == SUCCESS)
    	ESP_LOGI(TAG_AWS, "Shadow auto-reconnect enabled");
    else
    	ESP_LOGE(TAG_AWS, "Shadow auto-reconnect setup failure: %d ", rc);

    rc = aws_iot_shadow_register_delta(&aws_client, &lamp_actuator);
    if(rc == SUCCESS)
        ESP_LOGI(TAG_AWS, "Shadow Delta callback registered");
    else
     	ESP_LOGE(TAG_AWS, "Shadow Delta callback failed: %d ", rc);

    while(1)
    {
    	rc = aws_iot_shadow_yield(&aws_client, 100);
        if (rc != SUCCESS)
        	continue;

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

	vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
void aws_start(void)
{
	xTaskCreate(aws_iot_task, "aws_iot_task", 10240, (void *)0, 5, NULL);
}
//-----------------------------------------------------------------------------
void app_main(void)
{
	ESP_LOGI(TAG_MAIN, "Light Actuator v1.0 STARTED");
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_LOGI(TAG_MAIN, "Flash initialized");
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_LOGI(TAG_MAIN, "Event loop created");
	events_group = xEventGroupCreate();
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(LAMP_PIN, GPIO_MODE_OUTPUT);

	wifi_start();

	aws_start();
}
