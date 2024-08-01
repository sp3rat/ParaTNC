/*
 * event_log.h
 *
 *  Created on: May 26, 2024
 *      Author: mateusz
 */

#ifndef EVENT_LOG_H_
#define EVENT_LOG_H_

#include "stdint.h"

/// ==================================================================================================
///	GLOBAL MACROS
/// ==================================================================================================

#define EVENT_LOG_GET_SEVERITY(x) ((x & 0xF0) >> 4)

#define EVENT_LOG_GET_SOURCE(x) (x & 0x0F)

#define EVENT_LOG_SET_SEVERITY_SOURCE(severity, source) \
	(((uint8_t)severity & 0xF) << 4) | ((uint8_t)source & 0xF)

/// ==================================================================================================
///	GLOBAL DEFINITIONS
/// ==================================================================================================

#define EVENT_LOG_TIMESYNC_BOOTUP_WPARAM (0x77U)

/// ==================================================================================================
///	GLOBAL TYPEDEFS
/// ==================================================================================================


/**
 * TYpe used to distinguish between event of different severity
 */
typedef enum event_log_severity_t {
	EVENT_DEBUG,		/**< EVENT_DEBUG */
	EVENT_INFO,			/**< EVENT_INFO */
	EVENT_INFO_CYCLIC,
	EVENT_WARNING,		/**< EVENT_WARNING */
	EVENT_ERROR,		/**< EVENT_ERROR */
	EVENT_ASSERT,		/**< EVENT_ASSERT assert failure, which result in hard reset*/
	EVENT_BOOTUP,		/**< EVENT_BOOTUP all info events generated during bootup */
	EVENT_TIMESYNC, 	/**< EVENT_TIMESYNC event generated once at startup and then every 6 hours to
					   		   keep master_time and RTC date and time sync */
} event_log_severity_t;

/**
 * Where this long entry was generated
 */
typedef enum event_log_source_t {
	EVENT_SRC_MAIN,				 /**< EVENT_SRC_MAIN everywhere within main.c source file */
	EVENT_SRC_WX_HANDLER,		 /**< EVENT_SRC_WX_HANDLER */
	EVENT_SRC_PWR_SAVE,			 /**< EVENT_SRC_PWR_SAVE */
	EVENT_SRC_PACKET_TX_HANDLER, /**< EVENT_SRC_PACKET_TX_HANDLER */
	EVENT_SRC_APRSIS,			 /**< EVENT_SRC_APRSIS */
	EVENT_SRC_KISS,				 /**< EVENT_SRC_KISS */
	EVENT_SRC_APRS_RF,			 /**< EVENT_SRC_APRS_RF */
	EVENT_SRC_GSM_GPRS,			 /**< EVENT_SRC_GSM_GPRS */
	EVENT_SRC_TCPIP,			 /**< EVENT_SRC_TCPIP */
	EVENT_SRC_HTTP_CLIENT,		 /**< EVENT_SRC_HTTP_CLIENT */
	EVENT_SRC_MODBUS,			 /**< EVENT_SRC_MODBUS */
	EVENT_SRC_UMB,				 /**< EVENT_SRC_UMB */
	EVENT_SRC_DRV_ANEMOMETER,	 /**< EVENT_SRC_DRV_ANEMOMETER */
	EVENT_SRC_DRV_I2C,			 /**< EVENT_SRC_DRV_I2C */
	EVENT_SRC_DRV_UART,			 /**< EVENT_SRC_DRV_UART */
	EVENT_SRC_DRV_SPI,			 /**< EVENT_SRC_DRV_SPI */
} event_log_source_t;

/**
 * Structure used to store single system event in RAM2 area and Flash
 * non volatile storage
 */
typedef struct __attribute__ ((aligned (1))) event_log_t {
	uint32_t event_counter_id;	 //!< counter used to check which event is the oldest and newest one
	uint32_t event_master_time;	 //!< value of maser time at the moment an event is generated
	uint8_t severity_and_source; //!< high nibble -> severity level, low nibble -> source
	uint8_t event_id;			 //!< event id, unique across different sources & severity level
	uint8_t param;				 //!< Optional single-byte data, specific per event
	uint8_t param2;				 //!< Optional single-byte data, specific per event
	uint16_t wparam;			 //!< Optional 2-byte data, specific per event
	uint16_t wparam2;			 //!< Optional 2-byte data, specific per event
	uint32_t lparam;			 //!< Optional 4-byte data, specific per event
	uint32_t lparam2;			 //!< Optional 4-byte data, specific per event
} event_log_t;

/**
 * This is a structure, which holds single event log in exposed and non-packed form, with separate
 * fields for severity, source etc. Used to extract and then send events in APRS message, include to
 * HTTP rest service query, etc. 
 */
typedef struct event_log_exposed_t {
	uint32_t event_counter_id;	 //!< counter used to check which event is the oldest and newest one
	uint32_t event_master_time;	 //!< value of maser time at the moment an event is generated
	event_log_severity_t severity;
	event_log_source_t source;
	const char * source_str_name;//!< string representation of source name
	uint8_t event_id;			 //!< event id, unique across different sources & severity level
	const char * event_str_name; //!< string representation of the event
	uint8_t param;				 //!< Optional single-byte data, specific per event
	uint8_t param2;				 //!< Optional single-byte data, specific per event
	uint16_t wparam;			 //!< Optional 2-byte data, specific per event
	uint16_t wparam2;			 //!< Optional 2-byte data, specific per event
	uint32_t lparam;			 //!< Optional 4-byte data, specific per event
	uint32_t lparam2;			 //!< Optional 4-byte data, specific per event
}event_log_exposed_t;

/// ==================================================================================================
///	GLOBAL VARIABLES
/// ==================================================================================================


/// ==================================================================================================
///	GLOBAL FUNCTIONS
/// ==================================================================================================

/**
 * Initializes everything log related
 */
void event_log_init (void);

/**
 * Stores new event asynchronously. Events are written into all volatile, RAM mapped areas
 * immediately, but FLASH based areas are synchronized periodically.
 * @param severity
 * @param source
 * @param wparam
 * @param lparam
 * @param lparam2
 */
int8_t event_log (event_log_severity_t severity,
				event_log_source_t source,
				uint8_t event_id,
				uint8_t param,
				uint8_t param2,
				uint16_t wparam,
				uint16_t wparam2,
				uint32_t lparam,
				uint32_t lparam2);

/**
 * Stores an event synchronously to all targer areas
 * @param severity
 * @param source
 * @param event_id
 * @param param
 * @param param2
 * @param wparam
 * @param wparam2
 * @param lparam
 * @param lparam2
 * @return
 */
int8_t event_log_sync (event_log_severity_t severity,
					 event_log_source_t source,
					 uint8_t event_id,
					 uint8_t param,
					 uint8_t param2,
					 uint16_t wparam,
					 uint16_t wparam2,
					 uint32_t lparam,
					 uint32_t lparam2);

/**
 * Returns a pointer to a string representing event source
 * @param src
 * @return
 */
const char * event_log_source_to_str(event_log_source_t src);

/**
 *
 * @param source
 * @param event_id
 * @return
 */
const char * event_id_to_str(event_log_source_t source, uint8_t event_id);

/**
 * Generates string representation of given event log in exposed form
 * @param exposed pointer to an event to be converted
 * @param output char buffer to place a string into
 * @param output_ln maximum length of output string
 * @return length of assembled string
 */
uint16_t event_exposed_to_string(const event_log_exposed_t * exposed, char * output, uint16_t output_ln);

#endif /* EVENT_LOG_H_ */
