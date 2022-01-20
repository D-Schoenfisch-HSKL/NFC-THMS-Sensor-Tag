/*
 * timer.h
 *
 *  Created on: 31.03.2021
 *      Author: DavidSchönfisch
 */

#ifndef TIMER_H_
#define TIMER_H_


#include <stdbool.h>
#include <stdint.h>

bool init_ms_timer(void);
void timer_wait_ms(uint32_t delay_in_ms);

#endif /* TIMER_H_ */
