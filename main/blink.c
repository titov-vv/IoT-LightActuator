/*
 * blink.c
 *
 *  Created on: Oct 29, 2019
 *      Author: vtitov
 */

#include "blink.h"

#include "driver/gpio.h"
//-----------------------------------------------------------------------------
TaskHandle_t 	blink_task_handle = NULL;
uint32_t 		BlinkPattern = 0;
//-----------------------------------------------------------------------------
void set_blink_pattern(uint32_t pattern)
{
	xTaskNotify(blink_task_handle, pattern, eSetValueWithOverwrite);
}
//-----------------------------------------------------------------------------
void blink_task(void *arg)
{
	int	i, tick;
	uint32_t pattern = 0;

	while(1)
	{
		xTaskNotifyWait(0x00, 0x00, &BlinkPattern, 0);
		pattern = BlinkPattern;

		for (i=0; i<32; i++)
		{
			tick = pattern & 0x01;
			pattern = pattern >> 1;

			gpio_set_level(LED_PIN, tick);
			vTaskDelay(BLINK_TICK / portTICK_RATE_MS);
		}
	}

	vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
void blink_start(void)
{
	xTaskCreate(blink_task, "blink_task", 4096, (void *)0, 5, &blink_task_handle);
}
//-----------------------------------------------------------------------------
