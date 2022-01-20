/*
 * ads1115.h
 *
 *  Created on: 29.03.2021
 *      Author: DavidSchönfisch
 *     Version:	1.0
 */

#ifndef ADS1115_H_
#define ADS1115_H_

#include <stdbool.h>
#include <stdint.h>

#include "fsl_i2c.h"

/**************************************************************************/
/** <<<START: Macros, Symbols, Enums und Typedefs */

#define ADS1115 //#define ADS1115

#define ADS_DELTA_T_CORRECTION_TIME_US 			606 // Correction time for ADS1115 sampling (due to I2C communication etc...)

/*=========================================================================
    I2C ADDRESS/BITS (Ist einstellbar ueber change_I2C_address() )
    -----------------------------------------------------------------------*/
		typedef enum
		{
			ADDRESS_DEFAULT         = 0x48,    //ADDR = GND, Default
			ADDRESS_GND             = 0x48,    //100 1000
			ADDRESS_VDD             = 0x49,    //100 1001
			ADDRESS_SDA             = 0x50,    //100 1010
			ADDRESS_SCL             = 0x51     //100 1011
		}ADS1115_address_t;
/*=========================================================================*/

/*=========================================================================
    CONVERSION DELAY (in mS)
    -----------------------------------------------------------------------*/
    #define ADS1015_CONVERSIONDELAY         (1)
    #define ADS1115_CONVERSIONDELAY         (8)
/*=========================================================================*/

/*=========================================================================
    POINTER REGISTER
    -----------------------------------------------------------------------*/
		typedef enum
		{
			REG_POINTER_MASK        = 0x03,
			REG_POINTER_CONVERT     = 0x00,
			REG_POINTER_CONFIG      = 0x01,
			REG_POINTER_LOWTHRESH   = 0x02,
			REG_POINTER_HITHRESH    = 0x03
		}ADS1115_gegister_pointer_t;
/*=========================================================================*/

/*=========================================================================
    CONFIG REGISTER
    -----------------------------------------------------------------------*/
		typedef enum
		{
			 ADS1115_REG_CONFIG_OS_MASK      =    0x8000,
			 ADS1115_REG_CONFIG_OS_SINGLE    =    0x8000,  // Write: Set to start a single-conversion
			 ADS1115_REG_CONFIG_OS_BUSY      =    0x0000,  // Read: Bit = 0 when conversion is in progress
			 ADS1115_REG_CONFIG_OS_NOTBUSY   =    0x8000,  // Read: Bit = 1 when device is not performing a conversion

			 ADS1115_REG_CONFIG_MUX_MASK     =    0x7000,
			 ADS1115_REG_CONFIG_MUX_DIFF_0_1 =    0x0000,  // Differential P = AIN0, N = AIN1 =    default,
			 ADS1115_REG_CONFIG_MUX_DIFF_0_3 =    0x1000,  // Differential P = AIN0, N = AIN3
			 ADS1115_REG_CONFIG_MUX_DIFF_1_3 =    0x2000,  // Differential P = AIN1, N = AIN3
			 ADS1115_REG_CONFIG_MUX_DIFF_2_3 =    0x3000,  // Differential P = AIN2, N = AIN3
			 ADS1115_REG_CONFIG_MUX_SINGLE_0 =    0x4000,  // Single-ended AIN0
			 ADS1115_REG_CONFIG_MUX_SINGLE_1 =    0x5000,  // Single-ended AIN1
			 ADS1115_REG_CONFIG_MUX_SINGLE_2 =    0x6000,  // Single-ended AIN2
			 ADS1115_REG_CONFIG_MUX_SINGLE_3 =    0x7000,  // Single-ended AIN3

			 ADS1115_REG_CONFIG_PGA_MASK     =    0x0E00,
			 ADS1115_REG_CONFIG_PGA_6_144V   =    0x0000,  // +/-6.144V range = Gain 2/3
			 ADS1115_REG_CONFIG_PGA_4_096V   =    0x0200,  // +/-4.096V range = Gain 1
			 ADS1115_REG_CONFIG_PGA_2_048V   =    0x0400,  // +/-2.048V range = Gain 2 =    default,
			 ADS1115_REG_CONFIG_PGA_1_024V   =    0x0600,  // +/-1.024V range = Gain 4
			 ADS1115_REG_CONFIG_PGA_0_512V   =    0x0800,  // +/-0.512V range = Gain 8
			 ADS1115_REG_CONFIG_PGA_0_256V   =    0x0A00,  // +/-0.256V range = Gain 16

			 ADS1115_REG_CONFIG_MODE_MASK    =    0x0100,
			 ADS1115_REG_CONFIG_MODE_CONTIN  =    0x0000,  // Continuous conversion mode
			 ADS1115_REG_CONFIG_MODE_SINGLE  =    0x0100,  // Power-down single-shot mode =    default,

			 ADS1115_REG_CONFIG_DR_MASK      =    0x00E0,
			 ADS1115_REG_CONFIG_DR_1      	 =    0x0000,  // ADS1015: 128  / ADS1115: 8 samples per second
			 ADS1115_REG_CONFIG_DR_2     	 =    0x0020,  // ADS1015: 250  / ADS1115: 16 samples per second
			 ADS1115_REG_CONFIG_DR_3     	 =    0x0040,  // ADS1015: 490  / ADS1115: 32 samples per second
			 ADS1115_REG_CONFIG_DR_4     	 =    0x0060,  // ADS1015: 920  / ADS1115: 64 samples per second
			 ADS1115_REG_CONFIG_DR_5		 =    0x0080,  // ADS1015: 1600 / ADS1115: 128 samples per second =    default,
			 ADS1115_REG_CONFIG_DR_6    	 =    0x00A0,  // ADS1015: 2400 / ADS1115: 250 samples per second
			 ADS1115_REG_CONFIG_DR_7    	 =    0x00C0,  // ADS1015: 3300 / ADS1115: 475 samples per second
			 ADS1115_REG_CONFIG_DR_8   		 =    0x00E0,  // ADS1015: 3300 / ADS1115: 860 samples per second

			 ADS1115_REG_CONFIG_CMODE_MASK   =    0x0010,
			 ADS1115_REG_CONFIG_CMODE_TRAD   =    0x0000,  // Traditional comparator with hysteresis =    default,
			 ADS1115_REG_CONFIG_CMODE_WINDOW =    0x0010,  // Window comparator

			 ADS1115_REG_CONFIG_CPOL_MASK    =    0x0008,
			 ADS1115_REG_CONFIG_CPOL_ACTVLOW =    0x0000,  // ALERT/RDY pin is low when active =    default,
			 ADS1115_REG_CONFIG_CPOL_ACTVHI  =    0x0008,  // ALERT/RDY pin is high when active

			 ADS1115_REG_CONFIG_CLAT_MASK    =    0x0004,  // Determines if ALERT/RDY pin latches once asserted
			 ADS1115_REG_CONFIG_CLAT_NONLAT  =    0x0000,  // Non-latching comparator =    default,
			 ADS1115_REG_CONFIG_CLAT_LATCH   =    0x0004,  // Latching comparator

			 ADS1115_REG_CONFIG_CQUE_MASK    =    0x0003,
			 ADS1115_REG_CONFIG_CQUE_1CONV   =    0x0000,  // Assert ALERT/RDY after one conversions
			 ADS1115_REG_CONFIG_CQUE_2CONV   =    0x0001,  // Assert ALERT/RDY after two conversions
			 ADS1115_REG_CONFIG_CQUE_4CONV   =    0x0002,  // Assert ALERT/RDY after four conversions
			 ADS1115_REG_CONFIG_CQUE_NONE    =    0x0003,  // Disable the comparator and put ALERT/RDY in high state =    default,

			 ADS1115_REG_CONFIG_DEFAULT			 =		0x8583	 // Default Value of Config Register
		} ADS1115_config_register_t;

/*=========================================================================
    ENUMARTIONS and STRUCTS
    -----------------------------------------------------------------------*/
		typedef enum
		{
			ADS_GAIN_TWOTHIRDS, // +/-6.144V range = Gain 2/3
			ADS_GAIN_ONE,  		// +/-4.096V range = Gain 1
			ADS_GAIN_TWO,  		// +/-2.048V range = Gain 2 =    default,
			ADS_GAIN_FOUR,  	// +/-1.024V range = Gain 4
			ADS_GAIN_EIGHT,  	// +/-0.512V range = Gain 8
			ADS_GAIN_SIXTEEN, 	// +/-0.256V range = Gain 16
			ADS_NUMBER_OF_GAINS
		} adsPGA_t;

		typedef enum      // !!!!!!!!!!!!! MUSS FueR ADS1X15 ANGEPASST WERDEN
		{
			ADS_DR_1,  // ADS1015: 128  / ADS1115: 8 samples per second
			ADS_DR_2,  // ADS1015: 250  / ADS1115: 16 samples per second
			ADS_DR_3,  // ADS1015: 490  / ADS1115: 32 samples per second
			ADS_DR_4,  // ADS1015: 920  / ADS1115: 64 samples per second
			ADS_DR_5,  // ADS1015: 1600 / ADS1115: 128 samples per second (default)
			ADS_DR_6,  // ADS1015: 2400 / ADS1115: 250 samples per second
			ADS_DR_7,  // ADS1015: 3300 / ADS1115: 475 samples per second
			ADS_DR_8,  // ADS1015: 3300 / ADS1115: 860 samples per second
			ADS_NUMBER_OF_DATARATES
		} adsDR_t;

		typedef enum
		{
			Diff_0_1		 = ADS1115_REG_CONFIG_MUX_DIFF_0_1,  // Differential P = AIN0, N = AIN1 =    default,
			Diff_0_3		 = ADS1115_REG_CONFIG_MUX_DIFF_0_3,  // Differential P = AIN0, N = AIN3
			Diff_1_3		 = ADS1115_REG_CONFIG_MUX_DIFF_1_3,  // Differential P = AIN1, N = AIN3
			Diff_2_3		 = ADS1115_REG_CONFIG_MUX_DIFF_2_3,  // Differential P = AIN2, N = AIN3
			Single_0		 = ADS1115_REG_CONFIG_MUX_SINGLE_0,  // Single-ended AIN0
			Single_1		 = ADS1115_REG_CONFIG_MUX_SINGLE_1,  // Single-ended AIN1
			Single_2		 = ADS1115_REG_CONFIG_MUX_SINGLE_2,  // Single-ended AIN2
			Single_3		 = ADS1115_REG_CONFIG_MUX_SINGLE_3   // Single-ended AIN3
		}adsMUX_Config_t;

		typedef enum
		{
			Continuous_Conversion			= ADS1115_REG_CONFIG_MODE_CONTIN,
			Single_Short					= ADS1115_REG_CONFIG_MODE_SINGLE
		}adsConversionMode;


typedef uint32_t error_ads_t;

enum ads_errors_e{
	ADS_NO_ERROR 					= 0x00U,
	ADS_ERROR_I2C_COMMUNICATION		= 0x01U,
	ADS_ERROR_CLOCK					= 0x02U,
	ADS_ERROR_MEMORY_SIZE			= 0x02U,
	ADS_ERROR_NOT_YET_IMPLEMENTED	= 0xFEU,
	ADS_ERROR_UNSPECIFIC			= 0xFFU
};


/** >>>ENDE: Macros, Symbols, Enums und Typedefs */

/**************************************************************************/
/** <<<START: Deklaration externer Funktionen */

error_ads_t init_ADS1115(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p);
void    	ADS1115_changeI2CAddress(uint8_t i2cAddress);
void 		ADS1115_useAlertPin(bool useAlertPin, bool (*callbackReadAdsAlertPin)(void));
void		ADS1115_setMUX(adsMUX_Config_t muxConfig);
void		ADS1115_setGain(adsPGA_t adsPGA);
uint32_t	ADS1115_getGainAmplification_inpv(void);
void      	ADS1115_setDataRate(adsDR_t adsDR);
uint32_t 	ADS1115_getSamplingPeriod_us();
void		ADS1115_setConversionMode(adsConversionMode mode);	// Nicht notwendig als externe Funktion, da in getSingleConversion oder startContinuous aufgerufen wird
void		ADS1115_sendConfig(void); // Nicht notwendig als externe Funktion, da immer vor getSingleConversion wie auch startContinuous aufgerufen wird

int16_t		ADS1115_getSingleConversion(void);

void   		ADS1115_startContinuous(void);
void      	ADS1115_stopContiuous(void); // = powerDown()
int16_t   	ADS1115_getLastConversionResults(void);
void		ADS1115_get_THMS_TimeMeasurementValue_array(int32_t timeValues_inus[], int32_t measurementValues[], uint32_t * multiplierInPV_p , uint16_t length);



/** >>>ENDE: Deklaration externer Funktionen */





#endif /* ADS1115_H_ */

/** @} */

