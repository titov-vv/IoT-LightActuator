/*
 * wifi.h
 *
 *  Created on: Oct 29, 2019
 *      Author: vtitov
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
//-----------------------------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//-----------------------------------------------------------------------------
#define TAG_WIFI			"WiFi"
#define SNMP_SERVER_ADDRESS	"pool.ntp.org"
#define DEVICE_TIMEZONE		"MSK-3"
//-----------------------------------------------------------------------------
// Function to initiate WiFi connection background task
// events_group[wifi_bit] - where to notify when IP connection is up
// events_group[ready_bit] - where to nofiry that device is ready, time is set
void wifi_start(EventGroupHandle_t events_group, int wifi_bit, int ready_bit);
//-----------------------------------------------------------------------------
#endif /* MAIN_WIFI_H_ */
