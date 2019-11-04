/*
 * thing.h
 *
 *  Created on: Nov 1, 2019
 *      Author: vtitov
 */

#ifndef MAIN_THING_H_
#define MAIN_THING_H_
//-----------------------------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//-----------------------------------------------------------------------------
// THING SHADOW EXAMPLE
//{
//  "desired": {
//    "lampOn": false,
//    "night_start": "23:00",
//    "night_end": "07:00"
//  },
//  "reported": {
//    "lampOn": false,
//    "night_start": "23:00",
//    "night_end": "07:00"
//  }
//}
//-----------------------------------------------------------------------------
// Pin connected to MOSFET key that operates relay
#define LAMP_PIN	GPIO_NUM_13
//-----------------------------------------------------------------------------
#define TAG_AWS		"AWS"
//-----------------------------------------------------------------------------
// Function to initiate AWS IOT task and handle MQTT exchange with the Cloud
// events_group[wifi_bit] is using to track availability of IP connection
void aws_start(EventGroupHandle_t events_group, int wifi_bit);
//-----------------------------------------------------------------------------
#endif /* MAIN_THING_H_ */
