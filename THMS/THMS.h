/*
 * THMS.h
 *
 *  Created on: 31.03.2021
 *      Author: DavidSchönfisch
 */

#ifndef THMS_H_
#define THMS_H_

#include <stdbool.h>
#include <stdint.h>

#include "fsl_i2c.h"

/**************************************************************************/
/* >> START: Symbols, Enums and Macros */

#define PRINT_UART_THMS_DEBUG_INFO		false	//
#define PRINT_UART_PULSE_CURVE			true	// Print of pulse-curve-samples via UART

#define ADS_GAIN 				ADS_GAIN_EIGHT  // Gain of ADS. To be adjusted according to sensor temperature range (bridge-voltage range).
#define	ADS_DR					ADS_DR_4		// Data rate of ADS. (!Different for ADS1015 and ADS1115!)

/* >> END: Symbols, Enums and Macros */
/**************************************************************************/
/* >> START: External Functions */

/*************************************************************************************
 * Function to initialize the THMS measurement setup (includes initialization of ADS1X115).
 * @param[in] *base_p: Pointer to I2C base configuration for LPC812.
 * @param[in] *i2c_master_config_p: Pointer to I2C_master configuration for LPC812.
 *************************************************************************************/
void init_thms(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p);


/*************************************************************************************
 * Callback-Function to get the state of the Alert_Pin of the ADS1X15 (Measurement ready to readout indicatior).
 * To be handeled to ADS1115 functions within init_thms function.
 * @return: Status.
 *************************************************************************************/
bool thms_readAlertPinCheck(void);


/*************************************************************************************
 * Function to measure the THMS-Sensor-Signal.
 * @param[in] pulseLength_inms: Length of the desired THMS-heat-pulse.
 * @param[in] fitStart_inms: Start point for THMS-fit fin ms
 * @param[out] *p_SensorSignal: THMS Sensor-Signal in sqrt(ms)/V (to be checke??)
 * @param[out] *p_MeasurementSignal: Second ADC sample in uV. Can be used to calculate the ambient temperature.
 * @param[out] *p_rsqrt_indicator_by2pow32: Rsqrt (Coefficient of determination) multiplied with 2^32.
 * @param[out] char *ss_type_20: Pointer to char array (at least 20) to write type of sensor-signal.perature.
 * @param[out] char *ms_type_20: Pointer to char array (at least 20) to write type of measurement-signal.
 * @param[out) uint32_t * multiplierInPV_out_p: Multiplier to convert one LSB of ADC into voltage.
 *************************************************************************************/
void thms_get_sensor_signal(int32_t * p_SensorSignal, int32_t * p_MeasurementSignal, uint32_t * p_rsqrt_indicator, uint32_t pulseLenght_inms, uint32_t fitStart_inms, char ss_type_20[], char ms_type_20[], uint32_t * multiplierInPV_out_p);

/* >> END: External Functions */





#endif /* THMS_H_ */
