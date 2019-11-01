/*
 * wifi.c
 *
 *  Created on: Oct 29, 2019
 *      Author: vtitov
 */
//-----------------------------------------------------------------------------
#include "wifi.h"
#include "blink.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
// Here is a private file with WiFi connection details
// It contains defines for WIFI_SSID and WIFI_PASSWORD
#include "wifi_credentials.h"
//-----------------------------------------------------------------------------
static int	wifi_retry = 0;
static int	notification_bit;
static EventGroupHandle_t notification_group;
//-----------------------------------------------------------------------------
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
		case WIFI_EVENT_STA_START:
			set_blink_pattern(BLINK_FAST);
			ESP_LOGI(TAG_WIFI, "Connecting...");
			esp_wifi_connect();
			xEventGroupClearBits(notification_group, notification_bit);
			break;
		case WIFI_EVENT_STA_CONNECTED:
			wifi_retry = 0;
			ESP_LOGI(TAG_WIFI, "Connected");
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			wifi_retry++;
			set_blink_pattern(BLINK_FAST);
			ESP_LOGI(TAG_WIFI, "Reconnection attempt %d", wifi_retry);
			esp_wifi_connect();
			xEventGroupClearBits(notification_group, notification_bit);
			break;
		}
	}

	if (event_base == IP_EVENT)
	{
		switch(event_id)
		{
		case IP_EVENT_STA_GOT_IP:
			set_blink_pattern(BLINK_OFF);
			xEventGroupSetBits(notification_group, notification_bit);
			ESP_LOGI(TAG_WIFI, "Received IP: %s", ip4addr_ntoa(&((ip_event_got_ip_t*)event_data)->ip_info.ip));
			break;
		}
	}
}
//-----------------------------------------------------------------------------
void wifi_start(EventGroupHandle_t events_group, int wifi_bit)
{
	notification_bit = wifi_bit;
	notification_group = events_group;

	ESP_LOGI(TAG_WIFI, "Initialization started");
	tcpip_adapter_init();
	wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
// TODO
// ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
	ESP_LOGI(TAG_WIFI, "Initialization completed");

	ESP_LOGI(TAG_WIFI, "Connect to '%s'", WIFI_SSID);
	wifi_config_t wifi_cfg = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD }};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG_WIFI, "Connection start...");
}
//-----------------------------------------------------------------------------
