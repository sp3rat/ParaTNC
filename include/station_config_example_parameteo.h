/*
 * config.h
 *
 *  Created on: 03.07.2017
 *      Author: mateusz
 */

#ifndef STATION_CONFIG_H_
#define STATION_CONFIG_H_

//#define _POWERSAVE_NORMAL
#define _POWERSAVE_AGGRESIVE
/*  ------------------ */
/* 	MODES OF OPERATION */

#define _METEO				// Enable meteo station
//#define _DIGI				// Enable WIDE1-1 digipeater
//#define _DIGI_ONLY_789	// Limit digipeater to handle only -7, -8 and -9 SSIDs
//#define _VICTRON			// Enable support for Victron VE.Direct protocol

//#define	_GSM			// only for ParaMETEO

/* 	MODES OF OPERATION */
/*  ------------------ */

//#define PARATNC_HWREV_C
#define PARAMETEO

/* ---------------------------- */
/* 	WEATHER/METEO CONFIGURATION */



// If none of those three sources are chosen (uncommented) the software will use internal sensors

//#define _UMB_MASTER
//#define _DAVIS_SERIAL
#define _MODBUS_RTU		// use Modbus RTU slave devices as a external meteo data source. For more configuration
						// (slave ids, registers...) please look into MODBUS RTU CONFIGURATION section of this file

#define _INTERNAL_AS_BACKUP		// if defined ParaTNC will switch to internal sensors in case of
								// the communication with UMB/Dallas Serial/Modbus external sensors will hang up


//#define _DALLAS_AS_TELEM	// Use Dallas one-wire thermometer as a 5th telemetry channel
							// May be used even if _METEO is not enabled
#define _DALLAS_SPLIT_PIN		// Must be enabled for all ParaTNC hardware revisions


/******** 	INTERNAL SENSORS CONFIGURATION *****************/

//#define _ANEMOMETER_TX20	// Use TX20 as an internal anemometer
#define _ANEMOMETER_ANALOGUE	// Use analogue/mechanical (like Davis 6410) as an internal anemometr
#define _ANEMOMETER_PULSES_IN_10SEC_PER_ONE_MS_OF_WINDSPEED 10

#define _SENSOR_MS5611
//#define _SENSOR_BME280

#define _UMB_SLAVE_ID 	 		1
#define _UMB_SLAVE_CLASS 		8
#define _UMB_CHANNEL_WINDSPEED			460
#define _UMB_CHANNEL_WINDGUSTS			440
#define _UMB_CHANNEL_WINDDIRECTION		580
#define _UMB_CHANNEL_TEMPERATURE		100
#define _UMB_CHANNEL_QFE				300

/******** 	INTERNAL SENSORS CONFIGURATION *****************/

/*************** DATA SOURCES CONFIG ***********************/
#define _TEMPERATURE_INTERNAL
//#define _TEMPERATURE_UMB
//#define _TEMPERATURE_RTU
//#define _TEMPERATURE_DAVIS
//
#define _PRESSURE_INTERNAL
//#define _PRESSURE_UMB
//#define _PRESSURE_RTU
//#define _PRESSURE_DAVIS
//
#define _HUMIDITY_INTERNAL
//#define _HUMIDITY_UMB
//#define _HUMIDITY_RTU
//#define _HUMIDITY_DAVIS
//
//
#define _WIND_INTERNAL
//#define _WIND_UMB
//#define _WIND_RTU
//#define _WIND_FULL_RTU
//#define _WIND_DAVIS
/*************** DATA SOURCES CONFIG ***********************/


/* 	WEATHER/METEO CONFIGURATION */
/* ---------------------------- */

//#define _MUTE_RF	// TODO: Not yet implemented - This will make station RXonly and disable all data transmission
//#define _MUTE_OWN	// TODO: Not yet implemented - This will disable all self-generated packets (wx, telemetry, beacon)
					// and switch device to "pure" kiss TNC operation. Packets from PC will be transmitted normally.

// Coordines should be in APRS decimal format DDDMM.SS for Longitude and DDMM.SS for latitude
#define _CALL "SP8EBC"
#define _SSID 1
#define _LAT		4948.82
#define _LATNS		'N'
#define _LON		01903.50
#define _LONWE		'E'
#define _COMMENT	"Set Your configuration in this file and rename to station_config.h"

// You can use only one of these below defines to choose symbol. Meteo data are are always transmitted with blue WX symbol
//#define _SYMBOL_DIGI			// uncomment if you want digi symbol(green star with D inside)
#define _SYMBOL_WIDE1_DIGI	// uncomment if you want 'little' digi symbol (green star with digit 1 overlaid)
//#define _SYMBOL_HOUSE			// uncomment if you want house symbol
//#define _SYMBOL_RXIGATE		// uncomment if you want rxigate symbol (black diamond with R)
//#define _SYMBOL_IGATE			// uncomment if you want igate symol (black diamond with I)

// Or you can keep commented all symbol defines and choose custom one based on data from APRS symbols table
//#define _SYMBOL_F	'/'
//#define _SYMBOL_S	'#'

// Uncomment one of these two defines to choose what path You want. If you uncommend both of them or
// if you keep both commended path will be completely disabled. CALL-S>AKLPRZ:data
#define _WIDE1_PATH		// CALL-S>AKLPRZ,WIDE1-1:data
//#define _WIDE21_PATH	// CALL-S>AKLPRZ,WIDE2-1:data

// Comment this to disable beacon auto sending during startup (this can be risky if RF feedback occur)
//#define _BCN_ON_STARTUP

#define _WX_INTERVAL 4		// WX packet interval in minutes
#define _BCN_INTERVAL 45	// Own beacon interval in minutes

#define _PTT_PUSHPULL // Uncomment this if you want PTT line to work as Push-pull instead of Open Drain
#define _SERIAL_BAUDRATE 9600

// Transmitting delay
#define _DELAY_BASE 20	// * 50ms. For example setting 10 gives 500msec delay. Maximum value is 20
//#define _RANDOM_DELAY	// adds random delay TO fixed time set by _DELAY_BASE. This additional time can be
						// from 100ms up to 1 sec in 100ms steps. Values are drawn from samples going from ADC
						// so it is better to use Unsquelched output in radio to provide much more randomness
//After waiting time declared above ParaTNC will check DCD (Data Carrier Detect) flag, which works as some
//kind of semaphore. If radio channel is not occupied by any other transmission TX will be keyed up immediately,
//otherwise software will wait for clear conditions.

// Few IMPORTANT hints about setting transmit delay properly.
//
// Transmit delay is key parameter to maintain RF network free from packet losses and collisions. If your station will be
// installed on tall object, without any other digi's close to it, you can set _DELAY_BASE to very low value and disable
// _RANDOM_DELAY. If you wanna rather auxiliary station, witch should only fill gap in RF coverage in small area, then
// _DELAY_BASE parameter should be not less than 12 (600msec), the smallest range the higher _DELAY_BASE should be.
// Additionally for gapfillers (auxiliary stations) _RANDOM_DELAY schould be enabled.
//
// This delay will ensure that while other station will be transmitting repeated packets from mobile, Yours will keep
// always quiet and won't jam RF network. This greatly improve DCD based access to channel. Various controllers uses
// various lenght of preamble, some of them produce signal which might be impossible to decode by ParaTNC, so DCD
// is only one part of effective multiaccess to medium.

/* ---------------------------- */
/* 	MODBUS RTU CONFIGURATION    */

// scaling coefficients are used as follows
//
// 				 A * x ^ 2 + B * x + C
// real value = ---------------------------
//							D
//
// because of that D cannot be set to zero
#define _RTU_SLAVE_SPEED		9600u
#define _RTU_SLAVE_PARITY		0
#define _RTU_SLAVE_STOP_BITS	2

#define _RTU_SLAVE_ID_1				0x01
#define _RTU_SLAVE_FUNC_1			0x03
#define _RTU_SLAVE_ADDR_1			0x00
#define _RTU_SLAVE_LENGHT_1			0x01
#define _RTU_SLAVE_SCALING_A_1	0
#define _RTU_SLAVE_SCALING_B_1	1
#define _RTU_SLAVE_SCALING_C_1	0
#define _RTU_SLAVE_SCALING_D_1  10

#define _RTU_SLAVE_ID_2				0x01
#define _RTU_SLAVE_FUNC_2			0x03
#define _RTU_SLAVE_ADDR_2			0x01
//#define _RTU_SLAVE_LENGHT_2			0x01
#define _RTU_SLAVE_SCALING_A_2	0
#define _RTU_SLAVE_SCALING_B_2	1
#define _RTU_SLAVE_SCALING_C_2	0
#define _RTU_SLAVE_SCALING_D_2  1

#define _RTU_SLAVE_ID_3				0x01
#define _RTU_SLAVE_FUNC_3			0x03
#define _RTU_SLAVE_ADDR_3			0x02
//#define _RTU_SLAVE_LENGHT_3			0x01
#define _RTU_SLAVE_SCALING_A_3	0
#define _RTU_SLAVE_SCALING_B_3	1
#define _RTU_SLAVE_SCALING_C_3	0
#define _RTU_SLAVE_SCALING_D_3  1

#define _RTU_SLAVE_ID_4				0x01
#define _RTU_SLAVE_FUNC_4			0x03
#define _RTU_SLAVE_ADDR_4			0x03
//#define _RTU_SLAVE_LENGHT_4			0x01
#define _RTU_SLAVE_SCALING_A_4	0
#define _RTU_SLAVE_SCALING_B_4	1
#define _RTU_SLAVE_SCALING_C_4	0
#define _RTU_SLAVE_SCALING_D_4  1

#define _RTU_SLAVE_ID_5				0x00
#define _RTU_SLAVE_FUNC_5			0x00
#define _RTU_SLAVE_ADDR_5			0x03
//#define _RTU_SLAVE_LENGHT_4			0x01
#define _RTU_SLAVE_SCALING_A_5	0
#define _RTU_SLAVE_SCALING_B_5	1
#define _RTU_SLAVE_SCALING_C_5	0
#define _RTU_SLAVE_SCALING_D_5  1


#define _RTU_SLAVE_ID_6				0x00
#define _RTU_SLAVE_FUNC_6			0x00
#define _RTU_SLAVE_ADDR_6			0x00
//#define _RTU_SLAVE_LENGHT_4			0x01
#define _RTU_SLAVE_SCALING_A_6	0
#define _RTU_SLAVE_SCALING_B_6	1
#define _RTU_SLAVE_SCALING_C_6	0
#define _RTU_SLAVE_SCALING_D_6  1

//#define _RTU_SLAVE_TEMPERATURE_SOURCE 		1
#define _RTU_SLAVE_HUMIDITY_SOURCE			2
//#define _RTU_SLAVE_PRESSURE_SOURCE			3
//#define _RTU_SLAVE_WIND_DIRECTION_SORUCE	4
//#define _RTU_SLAVE_WIND_SPEED_SOURCE		4


/* 	MODBUS RTU CONFIGURATION    */
/* ---------------------------- */

// Do not touch this
#if defined (_SYMBOL_DIGI) && !defined (_SYMBOL_WIDE1_DIGI) && !defined (_SYMBOL_HOUSE) && !defined (_SYMOL_RXIGATE) &&\
	!defined (_SYMBOL_IGATE)
#define _SYMBOL_F	'/'
#define _SYMBOL_S	'#'
#elif !defined (_SYMBOL_DIGI) && defined (_SYMBOL_WIDE1_DIGI) && !defined (_SYMBOL_HOUSE) && !defined (_SYMOL_RXIGATE) &&\
	!defined (_SYMBOL_IGATE)
#define _SYMBOL_F	'1'
#define _SYMBOL_S	'#'
#elif !defined (_SYMBOL_DIGI) && !defined (_SYMBOL_WIDE1_DIGI) && defined (_SYMBOL_HOUSE) && !defined (_SYMOL_RXIGATE) &&\
	!defined (_SYMBOL_IGATE)
#define _SYMBOL_F	'/'
#define _SYMBOL_S	'-'
#elif !defined (_SYMBOL_DIGI) && !defined (_SYMBOL_WIDE1_DIGI) && !defined (_SYMBOL_HOUSE) && defined (_SYMOL_RXIGATE) &&\
	!defined (_SYMBOL_IGATE)
#define _SYMBOL_F	'I'
#define _SYMBOL_S	'&'
#elif !defined (_SYMBOL_DIGI) && !defined (_SYMBOL_WIDE1_DIGI) && !defined (_SYMBOL_HOUSE) && !defined (_SYMOL_RXIGATE) &&\
	defined (_SYMBOL_IGATE)
#define _SYMBOL_F	'R'
#define _SYMBOL_S	'&'
#elif !defined (_SYMBOL_F) && !defined (_SYMBOL_S)
#error "Missing symbol configuration in station_config.h"
#elif defined (_SYMBOL_F) && defined (_SYMBOL_S)
#else
#error "Wrong symbol configuration in station_config.h"
#endif
//#if defined (_METEO) && !defined (_DIGI)
//#define _DIGI
//#endif

#if defined(PARATNC_HWREV_A) && (defined (_METEO) || defined (_DALLAS_AS_TELEM)) && !defined(_DALLAS_SPLIT_PIN)
#define _DALLAS_SPLIT_PIN
#endif

#if defined(PARATNC_HWREV_B) && (defined (_METEO) || defined (_DALLAS_AS_TELEM)) && !defined(_DALLAS_SPLIT_PIN)
#define _DALLAS_SPLIT_PIN
#endif

#if defined(_ANEMOMETER_TX20) && defined(_ANEMOMETER_ANALOGUE)
#error "You cannot use two anemometers at once!!!"
#endif

#if defined(_MOBUS_RTU) && defined(_DAVIS_SERIAL)
#error "You cannot use modbus RTU devices and Davis weather station at once!!!"
#endif

#if !defined(_ANEMOMETER_TX20) && !defined(_ANEMOMETER_ANALOGUE) && !defined(_UMB_MASTER) && defined(_METEO)
#define _ANEMOMETER_TX20
#endif

#endif /* STATION_CONFIG_H_ */
