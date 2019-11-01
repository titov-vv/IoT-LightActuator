/*
 * thing.c
 *
 *  Created on: Nov 1, 2019
 *      Author: vtitov
 */
#include "thing.h"
#include "blink.h"

#include <string.h>
#include "aws_iot_mqtt_client_interface.h"
#include "aws_config.h"
#include "cJSON.h"
//-----------------------------------------------------------------------------
char AWS_host[255] = AWS_HOST;
uint32_t AWS_port = AWS_PORT;
//-----------------------------------------------------------------------------
// Global variable to keep Cloud connection reference
static AWS_IoT_Client aws_client;
static int wifi_notification_bit;
static EventGroupHandle_t notification_group;
//-----------------------------------------------------------------------------
void aws_disconnect_handler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(TAG_AWS, "MQTT Disconnected");
}
//-----------------------------------------------------------------------------
IoT_Error_t configure_mqtt(AWS_IoT_Client *client)
{
	IoT_Error_t res = FAILURE;
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;

	ESP_LOGI(TAG_AWS, "MQTT init started");
	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	mqttInitParams.pHostURL = AWS_host;
	mqttInitParams.port = AWS_port;
	mqttInitParams.pRootCALocation = aws_root_ca_pem;
	mqttInitParams.pDeviceCertLocation = certificate_pem_crt;
	mqttInitParams.pDevicePrivateKeyLocation = private_pem_key;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 10000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = aws_disconnect_handler;
	mqttInitParams.disconnectHandlerData = NULL;

	res = aws_iot_mqtt_init(&aws_client, &mqttInitParams);
	if (res != SUCCESS)
		ESP_LOGE(TAG_AWS, "MQTT init failure: %d", res);
	else
	    ESP_LOGI(TAG_AWS, "MQTT init success");

	return res;
}
//-----------------------------------------------------------------------------
IoT_Error_t connect_mqtt(AWS_IoT_Client *client)
{
	int i;
	IoT_Error_t res = FAILURE;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	ESP_LOGI(TAG_AWS, "Start MQTT connection. Wait for IP link.");
	xEventGroupWaitBits(notification_group, wifi_notification_bit, false, true, portMAX_DELAY);
	ESP_LOGI(TAG_AWS, "IP link is up");

	ESP_LOGI(TAG_AWS, "MQTT connect started");
	connectParams.keepAliveIntervalInSec = 60;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = AWS_CLIENTID;
	connectParams.clientIDLen = (uint16_t) strlen(AWS_CLIENTID);
	connectParams.isWillMsgPresent = false;

	res = aws_iot_mqtt_connect(client, &connectParams);
	if(res != SUCCESS)
	{
		ESP_LOGE(TAG_AWS, "MQTT connect error: %d. Sleep 5 sec.", res);
		vTaskDelay(5000 / portTICK_RATE_MS);
		return res;
	}
	ESP_LOGI(TAG_AWS, "MQTT connected");

	if (aws_iot_mqtt_autoreconnect_set_status(client, true) == SUCCESS)
		ESP_LOGI(TAG_AWS, "MQTT auto-reconnect enabled");
	else
		ESP_LOGE(TAG_AWS, "MQTT auto-reconnect setup failure: %d", res);

	return res;
}
//-----------------------------------------------------------------------------
void aws_iot_task(void *arg)
{
	IoT_Error_t res = FAILURE;

	if (configure_mqtt(&aws_client) != SUCCESS)
		vTaskDelete(NULL);

	while (1)
	{
		res = aws_iot_mqtt_yield(&aws_client, 100);

		switch(res)
		{
		case SUCCESS:
		case NETWORK_RECONNECTED:
			set_blink_pattern(BLINK_ON);
			break;
		case NETWORK_ATTEMPTING_RECONNECT:		// Automatic re-connect is in progress
			vTaskDelay(100 / portTICK_RATE_MS);
			continue;
			break;
		case NETWORK_DISCONNECTED_ERROR:		// No connection available and need to connect
		case NETWORK_RECONNECT_TIMED_OUT_ERROR:
			set_blink_pattern(BLINK_SLOW);
			ESP_LOGI(TAG_AWS, "No MQTT connection available.");
			connect_mqtt(&aws_client);
			continue;
			break;
		default:
			ESP_LOGE(TAG_AWS, "Unexpected error in main loop: %d", res);
			//vTaskDelete(NULL);
		}

		vTaskDelay(3000 / portTICK_RATE_MS);
	}

	vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
void aws_start(EventGroupHandle_t events_group, int wifi_bit)
{
	wifi_notification_bit = wifi_bit;
	notification_group = events_group;

	xTaskCreate(aws_iot_task, "aws_iot_task", 20480, (void *)0, 5, NULL);
}
//-----------------------------------------------------------------------------
