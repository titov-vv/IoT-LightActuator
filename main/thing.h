/*
 * thing.h
 *
 *  Created on: Nov 1, 2019
 *      Author: vtitov
 */

#ifndef MAIN_THING_H_
#define MAIN_THING_H_
//-----------------------------------------------------------------------------
// THING SHADOW EXAMPLE
//{
//  "desired": {
//    "lamp_status": 0,
//    "night_start": "23:00",
//    "night_end": "07:00"
//  },
//  "reported": {
//    "lamp_status": 1,
//    "night_start": "23:00",
//    "night_end": "07:00"
//  }
//}
//-----------------------------------------------------------------------------
// Pin connected to MOSFET key that operates relay
#define LAMP_PIN	GPIO_NUM_13
//-----------------------------------------------------------------------------
// Function to initiate AWS IOT task and handle MQTT exchange with the Cloud
// READY_BIT is used to track device readiness (IP is up and Time is set)
void aws_start();
//-----------------------------------------------------------------------------
#endif /* MAIN_THING_H_ */
