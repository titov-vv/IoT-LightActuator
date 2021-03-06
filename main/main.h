/*
 * main.h
 *
 *  Created on: Nov 5, 2019
 *      Author: vtitov
 */

#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
//-----------------------------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//-----------------------------------------------------------------------------
// Define TAGs for log messages
#define	TAG_MAIN	"APP"
#define TAG_WIFI	"WiFi"
#define TAG_AWS		"AWS"
//-----------------------------------------------------------------------------
// Bit to indicate IP link readiness
#define IP_UP_BIT		BIT0
// Bit to indicate operational readiness
#define READY_BIT		BIT1
// Bit to indicate when WiFi was lost and re-init started
#define WIFI_LOST_BIT	BIT2
//-----------------------------------------------------------------------------
// FreeRTOS event group to to synchronize between tasks
// Real definition is in main_iot_actuator.c
extern EventGroupHandle_t events_group;
//-----------------------------------------------------------------------------
#endif /* MAIN_MAIN_H_ */
