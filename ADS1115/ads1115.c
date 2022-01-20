/*
 * ads1115.c
 *
 *  Created on: 29.03.2021
 *      Author: DavidSchönfisch
 *     Version:	1.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "ads1115.h"
#include "math.h"

#include "fsl_i2c.h"
#include "pin_mux.h"
#include "board.h"

#include "timer.h"

/**************************************************************************/
/** <<<START: Macros, Symbols, Enums und Typedefs */

// I2C Specific (for Hardware Abstraction)
//#define I2C_HANDLE_p_t	(I2C_Type);				//from "fsl_i2c.h"
//#define I2C_CONFIG_t	(i2c_master_config_t);	//from "fsl_i2c.h"


// Ist für ADS115 und ADS1015 gleich. Bei ADS 1015 sind die unteren 4Bits immer 0). Somit keine Anpassung notwendig.
// ENUM von float leider nicht moeglich
//                                            		                    ADS1015  ADS1115
/* !!!!!! LPC812 besitzt kein FPU (Float nicht möglich :-(
#define LSB_AMPLIFICATION_6_144V   (187.5e-6f)   	//  +/- 6.144V  1 bit = 3mV      0.1875mV (default)
#define LSB_AMPLIFICATION_4_096V   (125e-6f)  		//  +/- 4.096V  1 bit = 2mV      0.125mV
#define LSB_AMPLIFICATION_2_048V   (62.5e-6f)  		//  +/- 2.048V  1 bit = 1mV      0.0625mV
#define LSB_AMPLIFICATION_1_024V   (31.25e-6f)  	//  +/- 1.024V  1 bit = 0.5mV    0.03125mV
#define LSB_AMPLIFICATION_0_512V   (15.625e-6f)  	//  +/- 0.512V  1 bit = 0.25mV   0.015625mV
#define LSB_AMPLIFICATION_0_256V   (7.8125e-6f)  	//  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
*/
#ifdef ADS1115
#define LSB_IN_PV_GAIN_TWOTHIRDS	(187500000U)   	//  +/- 6.144V  1 bit = 3mV      0.1875mV (default)
#define LSB_IN_PV_GAIN_ONE   		(125000000U)  		//  +/- 4.096V  1 bit = 2mV      0.125mV
#define LSB_IN_PV_GAIN_TWO   		( 62500000U)  		//  +/- 2.048V  1 bit = 1mV      0.0625mV
#define LSB_IN_PV_GAIN_FOUR   		( 31250000U)  	//  +/- 1.024V  1 bit = 0.5mV    0.03125mV
#define LSB_IN_PV_GAIN_EIGHT   		( 15625000U)  	//  +/- 0.512V  1 bit = 0.25mV   0.015625mV
#define LSB_IN_PV_GAIN_SIXTEEN   	(  7812500U)  	//  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
#endif
#ifdef ADS1015
#define LSB_IN_PV_GAIN_TWOTHIRDS	(3000000000U)   	//  +/- 6.144V  1 bit = 3mV      0.1875mV (default)
#define LSB_IN_PV_GAIN_ONE   		(2000000000U)  		//  +/- 4.096V  1 bit = 2mV      0.125mV
#define LSB_IN_PV_GAIN_TWO   		( 100000000U)  		//  +/- 2.048V  1 bit = 1mV      0.0625mV
#define LSB_IN_PV_GAIN_FOUR   		( 500000000U)  	//  +/- 1.024V  1 bit = 0.5mV    0.03125mV
#define LSB_IN_PV_GAIN_EIGHT   		( 250000000U)  	//  +/- 0.512V  1 bit = 0.25mV   0.015625mV
#define LSB_IN_PV_GAIN_SIXTEEN   	(  12500000U)  	//  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
#endif

typedef struct
{
	uint16_t		PGA_Register_Config;
	uint32_t		lsb_amplification_inpv;
}PGA_setting_struct_t;

const PGA_setting_struct_t PGA_Settings[ADS_NUMBER_OF_GAINS] = {
	{ADS1115_REG_CONFIG_PGA_6_144V,LSB_IN_PV_GAIN_TWOTHIRDS}, //GAIN_TWOTHIRDS
	{ADS1115_REG_CONFIG_PGA_4_096V,LSB_IN_PV_GAIN_ONE}, //GAIN_ONE
	{ADS1115_REG_CONFIG_PGA_2_048V,LSB_IN_PV_GAIN_TWO}, //GAIN_TWO
	{ADS1115_REG_CONFIG_PGA_1_024V,LSB_IN_PV_GAIN_FOUR}, //GAIN_FOUR
	{ADS1115_REG_CONFIG_PGA_0_512V,LSB_IN_PV_GAIN_EIGHT}, // GAIN_EIGHT
	{ADS1115_REG_CONFIG_PGA_0_256V,LSB_IN_PV_GAIN_SIXTEEN} //GAIN_SIXTEEN
};

#ifdef ADS1115 // !!! Auchtung, LPC812 kann kein Float
#define INTERVAL_DR_1		(uint32_t)(1e6/8)	// ADS1115: 8 samples per second
#define INTERVAL_DR_2		(uint32_t)(1e6/16)	// ADS1115: 16 samples per second
#define INTERVAL_DR_3		(uint32_t)(1e6/32)	// ADS1115: 32 samples per second
#define INTERVAL_DR_4		(uint32_t)(1e6/64)	// ADS1115: 64 samples per second
#define INTERVAL_DR_5		(uint32_t)(1e6/128)	// ADS1115: 128 samples per second
#define INTERVAL_DR_6		(uint32_t)(1e6/250)	// ADS1115: 250 samples per second
#define INTERVAL_DR_7		(uint32_t)(1e6/475)	// ADS1115: 475 samples per second
#define INTERVAL_DR_8		(uint32_t)(1e6/860)	// ADS1115: 860 samples per second
#endif
#ifdef ADS1015
#define INTERVAL_DR_1		(uint32_t)(1e6/128)	// ADS1115: 8 samples per second
#define INTERVAL_DR_2		(uint32_t)(1e6/250)	// ADS1115: 16 samples per second
#define INTERVAL_DR_3		(uint32_t)(1e6/490)	// ADS1115: 32 samples per second
#define INTERVAL_DR_4		(uint32_t)(1e6/920)	// ADS1115: 64 samples per second
#define INTERVAL_DR_5		(uint32_t)(1e6/1600)	// ADS1115: 128 samples per second
#define INTERVAL_DR_6		(uint32_t)(1e6/2400)	// ADS1115: 250 samples per second
#define INTERVAL_DR_7		(uint32_t)(1e6/3300)	// ADS1115: 475 samples per second
#define INTERVAL_DR_8		(uint32_t)(1e6/3300)	// ADS1115: 860 samples per second
#endif

typedef struct
{
	uint16_t	DR_Register_Config;
	uint32_t	sample_interval_us;
}DR_setting_struct_t;

const DR_setting_struct_t DR_Settings[ADS_NUMBER_OF_DATARATES] = {
	{ADS1115_REG_CONFIG_DR_1,INTERVAL_DR_1}, //DR_1
	{ADS1115_REG_CONFIG_DR_2,INTERVAL_DR_2}, //DR_2
	{ADS1115_REG_CONFIG_DR_3,INTERVAL_DR_3}, //DR_3
	{ADS1115_REG_CONFIG_DR_4,INTERVAL_DR_4}, //DR_4
	{ADS1115_REG_CONFIG_DR_5,INTERVAL_DR_5}, //DR_5 - Default
	{ADS1115_REG_CONFIG_DR_6,INTERVAL_DR_6}, //DR_6
	{ADS1115_REG_CONFIG_DR_7,INTERVAL_DR_7}, //DR_7
	{ADS1115_REG_CONFIG_DR_8,INTERVAL_DR_8} //DR_8
};

/*typedef struct
{
	float				pga_amplification;
	uint32_t			sample_interval_us;
	static bool			is_alertPin_set;
	static uint32_t		alertPin;
	static uint16_t		configRegister;
	static uint8_t 		ads_i2c_address;
}ads_configuration_t; */

/** >>>ENDE: Macros, Symbols, Enums und Typedefs */

/**************************************************************************/
/** <<<START: Static Variables */
static uint32_t				pga_amplification_inpv_m;
static uint32_t				sample_interval_us_m;
static uint32_t				deltaTCorrectionTimeUs_m;
static bool					useAlertPin_m = false;
static uint32_t				alertPin_m;
static uint16_t				configRegister_m;
static uint8_t 				i2c_address_m;
static uint32_t				conversionTime_inms_m = ADS1115_CONVERSIONDELAY;

//I2C Specific
static i2c_master_config_t *	i2c_master_config_p_m;
static I2C_Type *	i2c_type_base_p_m;
/** >>>ENDE: Static Variables */

/**************************************************************************/
/** <<<START: Prototypen interner Funktionen (statics prototypes) */
// I2C communication (for Hardware Abstraction)
static error_ads_t config_i2c(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p);
//static error_ads_t read_i2c(uint8_t * valArray_p, uint8_t length);
//static error_ads_t write_i2c(uint8_t * valArray_p, uint8_t length);
static error_ads_t read_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length);
static error_ads_t write_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length);
static void wait_ms(uint32_t delay_in_ms);
// AlertPin (for Hardware Abstraction)
static bool checkMeasurementDone(void) ;
static bool (*callbackReadAdsAlertPin_p)(void); //Pointer to Callback Function to read ADS Alert Pin

static void setRegister(ADS1115_gegister_pointer_t Reg, uint16_t RegVal);
static int16_t readRegister(ADS1115_gegister_pointer_t registerToRead);
/** >>>ENDE: Prototypen interner Funktionen (statics prototypes) */

/**************************************************************************/
/** <<<START: Externe Funktionen */
error_ads_t init_ADS1115(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p){
		error_ads_t error_indicator = ADS_NO_ERROR;
		configRegister_m = ADS1115_REG_CONFIG_DEFAULT;
		i2c_address_m = ADDRESS_DEFAULT;
		useAlertPin_m = false;
		ADS1115_setGain(ADS_GAIN_TWOTHIRDS);
		conversionTime_inms_m = ADS1115_CONVERSIONDELAY;
		ADS1115_setDataRate(ADS_DR_5);
		ADS1115_setMUX(Diff_0_1);
		deltaTCorrectionTimeUs_m = ADS_DELTA_T_CORRECTION_TIME_US;
		error_indicator = config_i2c(base_p,i2c_master_config_p);
		if (error_indicator) {return error_indicator;}
		error_indicator = init_ms_timer();
		return error_indicator;
}

void ADS1115_changeI2CAddress(ADS1115_address_t i2c_address) {
		i2c_address_m = i2c_address;
}

void ADS1115_useAlertPin(bool useAlertPin, bool (*callbackReadAdsAlertPin)(void)) {
	useAlertPin_m = useAlertPin;
	if(useAlertPin_m) {
		if(callbackReadAdsAlertPin != 0x00) { // Or NULL ??
			bool (*callbackReadAdsAlertPin_p)(void) = callbackReadAdsAlertPin;
		} else {
			useAlertPin_m = false;
			//ToDo: Error Return
		}
	}
}

void ADS1115_setMUX(adsMUX_Config_t muxConfig) {
		configRegister_m &= (~ADS1115_REG_CONFIG_MUX_MASK);  //Clear PGA Config
		configRegister_m |= muxConfig; //Set PGA Config
}

void ADS1115_setGain(adsPGA_t adsPGA) {
		PGA_setting_struct_t PGA_Setting = PGA_Settings[adsPGA];
		configRegister_m &= (~ADS1115_REG_CONFIG_PGA_MASK);  //Clear PGA Config
		configRegister_m |= PGA_Setting.PGA_Register_Config; //Set PGA Config
		pga_amplification_inpv_m = PGA_Setting.lsb_amplification_inpv;
}

void ADS1115_setXYSamplingCorrectionTime(uint32_t deltaTCorrectionTimeUs) {
		deltaTCorrectionTimeUs_m = deltaTCorrectionTimeUs;
}

uint32_t ADS1115_getGainAmplification_inpv(void) {
		return	pga_amplification_inpv_m;
}

void ADS1115_setDataRate(adsDR_t adsDR) {
		DR_setting_struct_t DR_Setting = DR_Settings[adsDR];
		configRegister_m &= (~ADS1115_REG_CONFIG_DR_MASK);  //Clear DR Config
		configRegister_m |= DR_Setting.DR_Register_Config; //Set DR Config
		sample_interval_us_m = DR_Setting.sample_interval_us;	//Set Interval time in us
}

uint32_t ADS1115_getSamplingPeriod_us() {
		return sample_interval_us_m;
}

void ADS1115_setConversionMode(adsConversionMode mode) {
		configRegister_m &= (~ADS1115_REG_CONFIG_MODE_MASK);  //Clear DR Config
		configRegister_m |= mode; //Set DR Config
}

void ADS1115_sendConfig(){
		setRegister(REG_POINTER_CONFIG, configRegister_m);
}

int16_t ADS1115_getSingleConversion(){
		ADS1115_setConversionMode(Single_Short);
		configRegister_m |= ADS1115_REG_CONFIG_OS_SINGLE;	//Trigger Single Conversion
		ADS1115_sendConfig();
		wait_ms(conversionTime_inms_m);
		return ADS1115_getLastConversionResults();
}

void ADS1115_startContinuous(){
		ADS1115_setConversionMode(Continuous_Conversion);
		configRegister_m &= (~ADS1115_REG_CONFIG_CQUE_MASK);
		configRegister_m |= ADS1115_REG_CONFIG_CQUE_1CONV; //To enable RDY Pin
		setRegister(REG_POINTER_HITHRESH,  0xFFFF);
		setRegister(REG_POINTER_LOWTHRESH, 0x0000);
		ADS1115_sendConfig();
}

void ADS1115_stopContiuous() {
		ADS1115_setConversionMode(Single_Short);
		ADS1115_sendConfig();
}

int16_t ADS1115_getLastConversionResults() {
		return (readRegister(REG_POINTER_CONVERT));
}

/*
 * Measures multiple values in given sample rRate (defined with setDataRate).
 * Writes Values into xy array. Time (10^-6s) | Value (in LSB) | multiplierInPV in pVolt
 * If Sampling Rate is above 128Sps. The timing with DataReady Pin should be used.
 */
void ADS1115_get_THMS_TimeMeasurementValue_array(int32_t timeValues_inus[], int32_t measurementValues[], uint32_t * multiplierInPV_p , uint16_t length) {
		//uint32_t sampleInterval = (uint32_t)sample_interval_us_m/1000000.0;
		*multiplierInPV_p = pga_amplification_inpv_m;
		ADS1115_startContinuous();
		int16_t ConversionResult;
		if(!useAlertPin_m) {wait_ms((sample_interval_us_m/2)/1000);}
		for (uint32_t i = 0; i<length; i++){
			if(useAlertPin_m) {
				while(checkMeasurementDone() != true){
				}
			} else {
				wait_ms((sample_interval_us_m-deltaTCorrectionTimeUs_m)/1000);
			}
			ConversionResult = ADS1115_getLastConversionResults();	// Pointer auf Convert Register muesste wahrscheinlich nur einmal gesetzt werden
			measurementValues[i] = (int32_t) ConversionResult;
			// if (ConversionResult&0x8000) {measurementValues[i] &= 0xFFFF0000;}
			timeValues_inus[i] = (int32_t) (i*sample_interval_us_m); //dt ist in uV. sqrt(1'000'000) = 1'000
		}
		ADS1115_stopContiuous();
}

/** >>>ENDE: Externe Funktionen */

/**************************************************************************/
/** <<<START: Iterne Funktionen (static)*/
// For Hardware Abstraction
error_ads_t config_i2c(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p) {
	i2c_type_base_p_m = base_p;
	i2c_master_config_p_m = i2c_master_config_p;
	return ADS_NO_ERROR;
}

static error_ads_t read_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length) {
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, i2c_address_m, kI2C_Write);
	if (reVal == kStatus_Success) {
	    reVal = I2C_MasterWriteBlocking(i2c_type_base_p_m, &command, 1, kI2C_TransferNoStopFlag);
	    if (reVal == kStatus_Success) {
	    	reVal = I2C_MasterRepeatedStart(i2c_type_base_p_m, i2c_address_m, kI2C_Read);
	    	if (reVal == kStatus_Success) {
	    		reVal = I2C_MasterReadBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	    	}
	    }
	}
	reVal &= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? ADS_NO_ERROR : ADS_ERROR_I2C_COMMUNICATION;
}

static error_ads_t write_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length) {
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, i2c_address_m, kI2C_Write);
	if (reVal == kStatus_Success) {
	    reVal = I2C_MasterWriteBlocking(i2c_type_base_p_m, &command, 1, kI2C_TransferNoStopFlag);
	    if (reVal == kStatus_Success) {
	    	reVal = I2C_MasterWriteBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	    }
	}
	reVal &= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? ADS_NO_ERROR : ADS_ERROR_I2C_COMMUNICATION;
}

static void wait_ms(uint32_t delay_in_ms) {
	timer_wait_ms(delay_in_ms);
}

static bool checkMeasurementDone(void) {
	// ADS Pulls Alert Output Low if measurement is complete
	//ToDo: Hier stimmt was nicht.
	return (GPIO_PinRead(GPIO, 0U, 13U)) ? false : true;
	//ToDo: Problem: Pointer läuft ins Leere;
	//return (callbackReadAdsAlertPin_p()) ? false : true;
}

static void setRegister(ADS1115_gegister_pointer_t Reg, uint16_t RegVal) {
		uint8_t txBytes[2] = {0,0};
		error_ads_t error_indicator = ADS_NO_ERROR;

		txBytes[0] = (uint8_t)(RegVal >> 8);								// MSB of send command
		txBytes[1] = (uint8_t)(RegVal & 0xFF);								// LSB of send command

		error_indicator = write_i2c_with_command(Reg, txBytes, sizeof(txBytes));
		//ToDo: Eroor Handling!
}

static int16_t readRegister(ADS1115_gegister_pointer_t registerToRead) {
		char rxBytes[2] = {0,0};
		error_ads_t error_indicator = ADS_NO_ERROR;
		error_indicator = read_i2c_with_command((char) registerToRead, rxBytes, 2);
		//ToDo: Eroor Handling!
		return (((int16_t) rxBytes[1]) | (((int16_t) rxBytes[0]) << 8 ));
}

/** >>>ENDE: Interne Funktionen(statics) */

