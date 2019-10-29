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
#define TAG_WIFI	"WiFi"
//-----------------------------------------------------------------------------
// Function to initiate WiFi connection background task
// events_group[bit] - where to notify when IP connection is up
void wifi_start(EventGroupHandle_t events_group, int bit);
//-----------------------------------------------------------------------------
#endif /* MAIN_WIFI_H_ */
