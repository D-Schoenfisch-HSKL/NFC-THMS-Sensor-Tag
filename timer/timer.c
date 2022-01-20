/*
 * timer.c
 *
 *  Created on: 31.03.2021
 *      Author: DavidSchönfisch
 */


#include <stdbool.h>
#include <stdint.h>

#include "board.h"

volatile uint32_t g_systickCounter = 0;


void SysTick_Handler(void) //"Callback Funktion" for Clock defined with "SysTick_Config" //!! No static
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}

bool init_ms_timer(void) {
	return SysTick_Config(SystemCoreClock / 1000U);
}

void timer_wait_ms(int delay_in_ms) {
    g_systickCounter = delay_in_ms + 1; //+1 Ansonten zu schnell
	while (g_systickCounter != 0U)
	{
	}
}
