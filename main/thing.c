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
#define MAX_JSON_SIZE		512
#define MAX_SERVER_TIMEOUT	30000
//-----------------------------------------------------------------------------
static bool update_needed = true;
static bool update_inprogress = false;
static uint32_t publish_time = 0;
static char AWS_host[255] = AWS_HOST;
static uint32_t AWS_port = AWS_PORT;
// Topic names should be static as it will be lost from stack after exit from suscribtion function
static const char update_topic[] = "$aws/things/" AWS_CLIENTID "/shadow/update";
static const char delta_topic[] = "$aws/things/" AWS_CLIENTID "/shadow/update/delta";
static const char accepted_topic[] = "$aws/things/" AWS_CLIENTID "/shadow/update/accepted";
static const char rejected_topic[] = "$aws/things/" AWS_CLIENTID "/shadow/update/rejected";
// Tags for faster distiguish between accept/reject status updates
static int		delta_tag = 0x01;
static int		accepted_tag = 0x02;
static int		rejected_tag = 0x03;
// Thing status variables that are mirrored in Shadow
static bool LampStatus = false;
static int night_start_hh = 23;
static int night_start_mm = 0;
static int night_end_hh = 7;
static int night_end_mm = 0;
//-----------------------------------------------------------------------------
// Global variable to keep Cloud connection reference
static AWS_IoT_Client aws_client;
static int wifi_notification_bit;
static EventGroupHandle_t notification_group;
//-----------------------------------------------------------------------------
static void aws_disconnect_handler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(TAG_AWS, "MQTT Disconnected");
	set_blink_pattern(0x11111111);
}
//-----------------------------------------------------------------------------
static void delta_callback(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData)
{
    ESP_LOGI(TAG_AWS, "Delta callback %.*s\n%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);
}
//-----------------------------------------------------------------------------
static void status_callback(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData)
{
	int topic_tag;

	topic_tag = *((int *)pData);
	ESP_LOGI(TAG_AWS, "Status callback %.*s\n%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);

	if (topic_tag == accepted_tag)
    	ESP_LOGI(TAG_AWS, "Shadow updated accepted");
	if (topic_tag == rejected_tag)
	{
    	ESP_LOGI(TAG_AWS, "Shadow updated rejected");
		update_needed = true;
	}
	update_inprogress = false;
}
//-----------------------------------------------------------------------------
static IoT_Error_t configure_mqtt(AWS_IoT_Client *client)
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
static IoT_Error_t connect_mqtt(AWS_IoT_Client *client)
{
	IoT_Error_t res = FAILURE;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	ESP_LOGI(TAG_AWS, "Start MQTT connection. Wait for IP link.");
	xEventGroupWaitBits(notification_group, wifi_notification_bit, false, true, portMAX_DELAY);
	ESP_LOGI(TAG_AWS, "IP link is up");

	ESP_LOGI(TAG_AWS, "MQTT connect started");
	set_blink_pattern(0x11111111);
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
// Subscribe to shadow update topic - Accepted, Rejected and Delta
static IoT_Error_t subscribe_topics(AWS_IoT_Client *client)
{
	IoT_Error_t res = FAILURE;

	ESP_LOGI(TAG_AWS, "Subscribe to MQTT topics");
	res = aws_iot_mqtt_subscribe(client, delta_topic, strlen(delta_topic), QOS0, delta_callback, &delta_tag);
	if (res != SUCCESS)
	{
		ESP_LOGE(TAG_AWS, "Failed to subscribe delta topic: %d ", res);
		return res;
	}
	ESP_LOGI(TAG_AWS, "MQTT subscribed to %s", delta_topic);
	res = aws_iot_mqtt_subscribe(client, accepted_topic, strlen(accepted_topic), QOS0, status_callback, &accepted_tag);
	if (res != SUCCESS)
	{
		ESP_LOGE(TAG_AWS, "Failed to subscribe accept topic: %d ", res);
		return res;
	}
	ESP_LOGI(TAG_AWS, "MQTT subscribed to %s", accepted_topic);
	res = aws_iot_mqtt_subscribe(client, rejected_topic, strlen(rejected_topic), QOS0, status_callback, &rejected_tag);
	if (res!= SUCCESS)
	{
		ESP_LOGE(TAG_AWS, "Failed to subscribe reject topic: %d ", res);
		return res;
	}
	ESP_LOGI(TAG_AWS, "MQTT subscribed to %s", rejected_topic);

	return res;
}
//-----------------------------------------------------------------------------
void update_shadow(AWS_IoT_Client *client)
{
	IoT_Error_t res = FAILURE;
	cJSON *root, *state, *reported;
	char JSON_buffer[MAX_JSON_SIZE];
	char time_buffer[6];

// Post following JSON for update
//        	{
//        	  "state": {
//        	    "reported": {
//    				"lampOn": false,
//    				"night_start": "23:00",
//    				"night_end": "07:00"
//        	    }
//        	  }
//        	}
	ESP_LOGI(TAG_AWS, "Report current state");
	root = cJSON_CreateObject();
	state = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "state", state);
	reported = cJSON_CreateObject();
	cJSON_AddItemToObject(state, "reported", reported);
	if (LampStatus)
		cJSON_AddNumberToObject(reported, "LampStatus", 1);
	else
		cJSON_AddNumberToObject(reported, "LampStatus", 0);
	sprintf(time_buffer, "%02d:%02d", night_start_hh, night_start_mm);
	cJSON_AddStringToObject(reported, "night_start", time_buffer);
	sprintf(time_buffer, "%02d:%02d", night_end_hh, night_end_mm);
	cJSON_AddStringToObject(reported, "night_end", time_buffer);
	if (!cJSON_PrintPreallocated(root, JSON_buffer, MAX_JSON_SIZE, 0 /* not formatted */))
	{
		ESP_LOGW(TAG_AWS, "JSON buffer too small");
	    JSON_buffer[0] = 0;
	}
	cJSON_Delete(root);
	ESP_LOGI(TAG_AWS, "JSON Thing reported state:\n%s", JSON_buffer);

	ESP_LOGI(TAG_AWS, "MQTT publish to: %s", update_topic);
	IoT_Publish_Message_Params paramsQOS0;
    paramsQOS0.qos = QOS0;
    paramsQOS0.isRetained = 0;
    paramsQOS0.payload = (void *) JSON_buffer;
    paramsQOS0.payloadLen = strlen(JSON_buffer);
    res = aws_iot_mqtt_publish(client, update_topic, strlen(update_topic), &paramsQOS0);
    if (res == SUCCESS)
    {
    	publish_time = xTaskGetTickCount() * portTICK_RATE_MS;
    	ESP_LOGI(TAG_AWS, "MQTT message published @%d", publish_time);
    	update_needed = false;
    	update_inprogress = true;
    }
    else
      	ESP_LOGE(TAG_AWS, "MQTT publish failure: %d ", res);
}
//-----------------------------------------------------------------------------
void aws_iot_task(void *arg)
{
	IoT_Error_t res = FAILURE;
	int retry_cnt = 0;

	if (configure_mqtt(&aws_client) != SUCCESS)
		vTaskDelete(NULL);

	while (1)
	{
		res = aws_iot_mqtt_yield(&aws_client, 100);

		switch(res)
		{
		case SUCCESS:
		case NETWORK_RECONNECTED:
			retry_cnt = 0;
			set_blink_pattern(BLINK_OFF);
			if (update_inprogress)
			{
				if (((xTaskGetTickCount() * portTICK_RATE_MS) - publish_time) > MAX_SERVER_TIMEOUT)
				{
					update_inprogress = false;
					update_needed = true;
					ESP_LOGW(TAG_AWS, "Shadow update timeout");
				}
			}
			else
			{
				if (update_needed)
					update_shadow(&aws_client);
			}
			break;
		case NETWORK_ATTEMPTING_RECONNECT:		// Automatic re-connect is in progress
			vTaskDelay(100 / portTICK_RATE_MS);
			continue;
			break;
		case NETWORK_DISCONNECTED_ERROR:		// No connection available and need to connect
		case NETWORK_RECONNECT_TIMED_OUT_ERROR:
			ESP_LOGW(TAG_AWS, "No MQTT connection available.");
			update_inprogress = false;
			if (connect_mqtt(&aws_client) == SUCCESS)
			{
				if (subscribe_topics(&aws_client) != SUCCESS)
				{
					ESP_LOGE(TAG_AWS, "MQTT disconnected due to failed subscription");
					aws_iot_mqtt_disconnect(&aws_client);
					vTaskDelay(60000 / portTICK_RATE_MS); // sleep for 1 minute
				}
				else
					update_needed = true;

			}
			else
			{
				retry_cnt++;
				vTaskDelay(2000*retry_cnt / portTICK_RATE_MS);
				if (retry_cnt > 32)
				{
					retry_cnt = 0;
					vTaskDelay(3600000 / portTICK_RATE_MS); // suspend activity for 1 hour
				}
			}
			continue;
			break;
		default:
			ESP_LOGE(TAG_AWS, "Unexpected error in main loop: %d", res);
			//vTaskDelete(NULL);
		}

		vTaskDelay(100 / portTICK_RATE_MS);
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