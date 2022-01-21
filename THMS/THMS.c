/*
 * THMS.c
 *
 *  Created on: 31.03.2021
 *      Author: DavidSchönfisch
 */

#include <stdbool.h>
#include <stdint.h>
//#include <math.h> // Program Flash needy

#include "pin_mux.h"
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_i2c.h"

#include "THMS.h"
#include "timer.h"
#include "NTAG.h"
#include "ads1115.h"

#if PRINT_UART_THMS_DEBUG_INFO||PRINT_UART_PULSE_CURVE
	#include "fsl_debug_console.h"
#endif

/*>>>------------------------------------------------------------*/
/* >> START: Symbols, Enums, Macros & Const Values */
#define GPIO_PORT		0U		//Richtig? Gibt es überhaupt mehrere?
#define THMS_PULSE_PIN	7U		// Pulse Pin to control THMS-Heating (= Pin 3 im SO20 Packet)
#define ADS_ALERT_PIN	13U		// Alert Pin for ADS Controller (=Pin 2 im SO20 Packet)


/* >> END: Symbols, Enums, Macros & Const Values */

/*>>>------------------------------------------------------------*/
/* >> START: Local Variables */


/* >> END: Local Variables */


/*>>>------------------------------------------------------------*/
/* >> START: Prototypes (Internal Functions) */

/* Simple Linear-Regression Function
 * Makes a simple linear regression between two arrays x & y.
 * Both arrays must have same length.
 * Use array_length for length of whole array (x & y),
 * start_index for first element in arrays for fit start (offset).
 * Returns slope b1, offset b0 and Rpow2 (given in *2^32).
 */
static uint32_t mySQRTfun(uint32_t number);		// To avoid "math.h" Lib (>> Flash size problem)
static void Linear_Regression(int32_t x[], int32_t y[], uint8_t array_length, uint8_t start_index, int32_t *b1, int32_t *b0, uint32_t *Rpow2by2pow32);

/* >> END: Prototypes */


/*>>>------------------------------------------------------------*/
/* >> START: External Functions */

/*************************************************************************************
 * Function to initialize the THMS measurement setup (includes initialization of ADS1X115).
 * @param[in] *base_p: Pointer to I2C base configuration for LPC812.
 * @param[in] *i2c_master_config_p: Pointer to I2C_master configuration for LPC812.
 *************************************************************************************/
void init_thms(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p){
	error_ads_t ads_error;
	GPIO_PortInit(GPIO, GPIO_PORT);
    /* Init THMS PULSE PIN. Output + Low. */
	gpio_pin_config_t thms_pulse_pin_config = {kGPIO_DigitalOutput, 0};
	GPIO_PinInit(GPIO, GPIO_PORT, THMS_PULSE_PIN, &thms_pulse_pin_config);
	GPIO_PortClear(GPIO, GPIO_PORT, 1u << THMS_PULSE_PIN); //Set Pin to 0

    /* Init ADS_ALERT_PIN. Input. */
	gpio_pin_config_t ads_alert_pin_config = {kGPIO_DigitalInput, 0};
	GPIO_PortSet(GPIO, GPIO_PORT, 1u << ADS_ALERT_PIN); //Set Pin to 1
	GPIO_PinInit(GPIO, GPIO_PORT, ADS_ALERT_PIN, &ads_alert_pin_config);
	ads_error = init_ADS1115(base_p, i2c_master_config_p);
	ADS1115_useAlertPin(true, thms_readAlertPinCheck);
	ADS1115_setDataRate(ADS_DR); //=128, ADS1015-> DR_1 | ADS1115 -> DR5
	ADS1115_setGain(ADS_GAIN); //Besser ADS_GAIN_SIXTEEN
#if PRINT_UART_THMS_DEBUG_INFO
	if(ads_error) {PRINTF("Error at THMS Initialization \r\n");}
	else {PRINTF("THMS Initialization OK\r\n");};
#endif
}

/*************************************************************************************
 * Callback-Function to get the state of the Alert_Pin of the ADS1X15 (Measurement ready to readout indicatior).
 * To be handeled to ADS1115 functions within init_thms function.
 * @return: Status.
 *************************************************************************************/
bool thms_readAlertPinCheck(void) {
	// ADS Pulls Alert Output Low if measurement is complete
	if(GPIO_PinRead(GPIO, GPIO_PORT, ADS_ALERT_PIN)) {
		return 0;
	}
	return 1;
}

/*************************************************************************************
 * Function to measure the THMS-Sensor-Signal.
 * @param[in] pulseLength_inms: Length of the desired THMS-heat-pulse.
 * @param[in] fitStart_inms: Start point for THMS-fit fin ms
 * @param[out] *p_SensorSignal: THMS Sensor-Signal in sqrt(ms)/V (to be checke??)
 * @param[out] *p_MeasurementSignal: Second ADC sample in nV. Can be used to calculate the ambient temperature.
 * @param[out] char *ss_type_20: Pointer to char array (at least 20) to write type of sensor-signal.perature.
 * @param[out] char *ms_type_20: Pointer to char array (at least 20) to write type of measurement-signal.
 * ToDo: Checken ob multiplierInPV fehlerhaft? Um 10 zu gross?
 * @param[out] *p_rsqrt_indicator_by2pow32: Rsqrt (Coefficient of determination) multiplied with 2^32.
 *************************************************************************************/
void thms_get_sensor_signal(int32_t * p_SensorSignal, int32_t * p_MeasurementSignal, uint32_t * p_rsqrt_indicator_by2pow32, uint32_t pulseLenght_inms, uint32_t fitStart_inms, char ss_type_20[], char ms_type_20[]) {
	uint32_t samplingPeriodInus = ADS1115_getSamplingPeriod_us();
	uint16_t sampleCountIndex = (uint16_t) ((pulseLenght_inms*1000)/samplingPeriodInus);
	uint16_t fitStartIndex = (uint16_t) ((fitStart_inms*1000)/samplingPeriodInus);

#if PRINT_UART_THMS_DEBUG_INFO
	PRINTF("START THMS Measurement\r\n");
	PRINTF("SampleCountIndex: %d, FitStartIndex: %d\r\n",sampleCountIndex,fitStartIndex);
#endif

	int32_t timeArray[sampleCountIndex];
	int32_t valueArray[sampleCountIndex];

	GPIO_PortSet(GPIO, GPIO_PORT, 1u << THMS_PULSE_PIN); //Set Pin to 1
	uint32_t multiplierInPV;
    ADS1115_get_THMS_TimeMeasurementValue_array(timeArray,valueArray,&multiplierInPV,sampleCountIndex); //Time Array in us
	GPIO_PortClear(GPIO, GPIO_PORT, 1u << THMS_PULSE_PIN); //Set Pin to 0

	// Convert timeArry in us to sqrt(ns)
	for (uint32_t i = 0U; i < sampleCountIndex; i++) {
        timeArray[i] = mySQRTfun(timeArray[i]*1000); //Conversion to sqrt(ns)
    }

#if PRINT_UART_PULSE_CURVE
	/// Achtung! Über PRINTF werden keine negativen Zahlen ausgegeben bzw. kein Minuszeichen
	PRINTF("t [sqrt(ns)]:");
    for (int i = 0U; i < sampleCountIndex; i++) {
        PRINTF("%d ;", timeArray[i]);
    }
    PRINTF("\r\n");
	PRINTF("MS [LSB]:");
    for (int i = 0U; i < sampleCountIndex; i++)    {
    	PRINTF("%x ;", valueArray[i]);
    }
    PRINTF("\r\n");
	PRINTF("MS [LSB]:");
    for (int i = 0U; i < sampleCountIndex; i++)    {
        int32_t sensorVoltVal = valueArray[i];//((int32_t)multiplierInPV/1000)*valueArray[i];
    	PRINTF("%d ;", sensorVoltVal);
    }
    PRINTF("\r\n");
#endif
    uint32_t Rpow2by2pow32 = 0;
    int32_t b0 = 0;
    int32_t b1 = 0;


    Linear_Regression(valueArray,timeArray,sampleCountIndex,fitStartIndex,&b1,&b0,&Rpow2by2pow32);
    *p_SensorSignal = b1;
    memcpy(ss_type_20,"sqrt(ns)/LSB\0",13);
    //*p_SensorSignal = (1e9/multiplierInPV)*b1; // Convert sqrt(ns)/LSB to sqrt(ms)/V (sqrt(1e6) = 1e3 | 1e12/1e3 = 1e9)
	//ADS_GAIN_EIGHT -> multiplier=1562500 -> (1e9/multiplierInPV) = 64
	*p_rsqrt_indicator_by2pow32 = Rpow2by2pow32;
	*p_MeasurementSignal = valueArray[1]*((int32_t)multiplierInPV/1000); // > Second sample should be stable (first is usually incorrect)
    memcpy(ms_type_20,"nV\0",3);

#if PRINT_UART_PULSE_CURVE
	PRINTF("SS [sqrt(ns)/LSB]: %d\r\n",*p_SensorSignal); //Vorher [sqrt(ms)/V]
	PRINTF("MS [uV]: %d\r\n",*p_MeasurementSignal);
	PRINTF("1LSB [pV]: %d\r\n",multiplierInPV);
	PRINTF("b1 [sqrt(ns)/LSB]: %d\r\n",b1);
	PRINTF("RS: %u\r\n",*p_rsqrt_indicator_by2pow32);
#endif
}
/* >> END: External Functions */


/*>>>------------------------------------------------------------*/
/* >> START: Internal (Static) Functions */

static uint32_t mySQRTfun(uint32_t number) {
	uint32_t sqrt = number/2;
	uint32_t temp = 0;
	while(sqrt != temp){
		temp = sqrt;
		sqrt = (number/temp + temp)/2;
	}
	return sqrt;
}

static void Linear_Regression(int32_t x[], int32_t y[], uint8_t array_length, uint8_t start_index, int32_t *b1, int32_t *b0, uint32_t *Rpow2by2pow32) {
	int32_t xm = 0;
	int32_t ym = 0;
    for (uint8_t i = start_index; i < array_length; i++) {
        xm += x[i];
        ym += y[i];
    }
    xm = xm/(array_length-start_index);
    ym = ym/(array_length-start_index);
    int32_t Cov = 0;
    int32_t Var = 0;
    for (uint8_t i = start_index; i < array_length; i++) {
        Cov += (x[i]-xm)*(y[i]-ym);
        Var += (x[i]-xm)*(x[i]-xm); //pow(x[i]-xm,2);
    }
    *b1 = (Cov/Var);
    *b0 = ym - (*b1)*xm;
    uint32_t SQR = 0;
    uint32_t SQT = 0;

    for (uint8_t i = start_index; i < array_length; i++) {
        SQT += (uint32_t) (y[i]-ym)*(y[i]-ym);//pow(y[i]-ym,2);
        int32_t yp =(*b0) + ((*b1) * x[i]);
        SQR += (uint32_t) (y[i]-yp)*(y[i]-yp);//pow(y[i]-yp,2);
    }
    *Rpow2by2pow32 = 0xFFFFFFFF - SQR*(0xFFFFFFFF/SQT) ;//*Rpow2 = 1 - SQR/SQT;
}


/* >> END: Internal (Static) Functions */






