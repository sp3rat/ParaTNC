/*
 * wx_handler.c
 *
 *  Created on: 26.01.2019
 *      Author: mateusz
 */

#include "wx_handler.h"

#include <rte_wx.h>
#include <rte_rtu.h>
#include <rte_main.h>
#include <math.h>
#include <stm32f10x.h>
#include "drivers/_dht22.h"
#include "drivers/ms5611.h"
#include "drivers/bme280.h"
#include "drivers/analog_anemometer.h"
#include "drivers/tx20.h"

#include "station_config.h"

#include "config_data.h"

#ifdef _MODBUS_RTU
#include "modbus_rtu/rtu_getters.h"
#include "modbus_rtu/rtu_return_values.h"
#endif

#include "delay.h"
#include "telemetry.h"
#include "main.h"

#define WX_WATCHDOG_PERIOD (SYSTICK_TICKS_PER_SECONDS * SYSTICK_TICKS_PERIOD * 90)
#define WX_WATCHDOG_RESET_DURATION (SYSTICK_TICKS_PER_SECONDS * SYSTICK_TICKS_PERIOD * 3)

#define WX_MAX_TEMPERATURE_SLEW_RATE 4.0f

uint32_t wx_last_good_temperature_time = 0;
uint32_t wx_last_good_wind_time = 0;
wx_pwr_state_t wx_pwr_state;
uint8_t wx_inhibit_slew_rate_check = 1;
uint32_t wx_wind_pool_call_counter = 0;
uint8_t wx_force_i2c_sensor_reset = 0;

static const float direction_constant = M_PI/180.0f;

#define MODBUS_QF_TEMPERATURE_FULL		1
#define MODBUS_QF_TEMPERATURE_DEGR		(1 << 1)
#define MODBUS_QF_TEMPERATURE_NAVB		(1 << 2)
#define MODBUS_QF_HUMIDITY_FULL 		(1 << 3)
#define MODBUS_QF_HUMIDITY_DEGR 		(1 << 4)
#define MODBUS_QF_PRESSURE_FULL			(1 << 5)
#define MODBUS_QF_PRESSURE_DEGR			(1 << 6)

void wx_check_force_i2c_reset(void) {

	if (wx_force_i2c_sensor_reset == 1) {
		wx_force_i2c_sensor_reset = 0;

#if defined (_SENSOR_BME280)
		 bme280_reset(&rte_wx_bme280_qf);
		 bme280_setup();
#endif

#if defined (_SENSOR_MS5611)
		 ms5611_reset(&rte_wx_ms5611_qf);
		 ms5611_trigger_measure(0, 0);
#endif
	}

}

void wx_get_all_measurements(void) {

	int8_t j = 0;
	int32_t i = 0;
	int32_t return_value = 0;
	int8_t modbus_qf = 0;
	float pressure_average_sum = 0.0f;

//#if defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && !defined(_MODBUS_RTU)
	if (config_data_mode.wx_umb == 1) {
		if (rte_wx_umb_qf == UMB_QF_FULL) {
			rte_wx_temperature_average_dallas_valid = umb_get_temperature();
			rte_wx_pressure_valid = umb_get_qfe();
		}
	}
//#endif

#if !defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && defined(_MODBUS_RTU)

	#ifdef _RTU_SLAVE_TEMPERATURE_SOURCE
	return_value = rtu_get_temperature(&rte_wx_temperature_average_dallas_valid);

	// if temperature has been uploaded by the modbus sensor correctly
	if (return_value == MODBUS_RET_OK) {
		// update the last measurement timestamp to prevent relay clicking
		rte_wx_update_last_measuremenet_timers(RTE_WX_MEASUREMENT_TEMPERATURE);

		// set the first bit to signalize QF_FULL
		modbus_qf |= MODBUS_QF_TEMPERATURE_FULL;
	}
	else if (return_value == MODBUS_RET_DEGRADED) {
		// update the last measurement timestamp to prevent relay clicking
		rte_wx_update_last_measuremenet_timers(RTE_WX_MEASUREMENT_TEMPERATURE);

		// set the second bit to signalize QF_DEGRADED
		modbus_qf |= MODBUS_QF_TEMPERATURE_DEGR;
	}
	else {
		// set third bit if there is something wrong (like not avaliable or
		// not configured
		modbus_qf |= MODBUS_QF_TEMPERATURE_NAVB;
	}
	#endif

	// modbus rtu HUMIDITY
	#ifdef _RTU_SLAVE_HUMIDITY_SOURCE
	return_value = rtu_get_humidity(&rte_wx_humidity_valid);

	// do simmilar things but for humidity
	if (return_value == MODBUS_RET_OK) {
		modbus_qf |= MODBUS_QF_HUMIDITY_FULL;
	}
	else if (return_value == MODBUS_RET_DEGRADED) {
		modbus_qf |= MODBUS_QF_HUMIDITY_DEGR;
	}
	else {
		;
	}
	#endif

	// modbus rtu PRESSURE
	#ifdef _RTU_SLAVE_PRESSURE_SOURCE
	return_value = rtu_get_pressure(&rte_wx_pressure_valid);

	// do simmilar things but for pressure
	if (return_value == MODBUS_RET_OK) {
		modbus_qf |= MODBUS_QF_PRESSURE_FULL;
	}
	else if (return_value == MODBUS_RET_DEGRADED) {
		modbus_qf |= MODBUS_QF_PRESSURE_DEGR;
	}
	else {
		;
	}
	#endif
#endif

#if (!defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && !defined(_MODBUS_RTU) && defined (_SENSOR_MS5611)) || (defined (_SENSOR_MS5611) && defined (_MODBUS_RTU))
	// quering MS5611 sensor for temperature
	return_value = ms5611_get_temperature(&rte_wx_temperature_ms, &rte_wx_ms5611_qf);

	if (return_value == MS5611_OK) {
		rte_wx_temperature_ms_valid = rte_wx_temperature_ms;

	}

#endif

#if (!defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && !defined(_MODBUS_RTU) && defined (_SENSOR_BME280)) || (defined (_SENSOR_BME280) && defined (_MODBUS_RTU))

	// reading raw values from BME280 sensor
	return_value = bme280_read_raw_data(bme280_data_buffer);

	if (return_value == BME280_OK) {

		// setting back the Quality Factor to FULL to trace any problems with sensor readouts
		rte_wx_bme280_qf = BME280_QF_FULL;

		// converting raw values to temperature
		bme280_get_temperature(&rte_wx_temperature_ms, bme280_get_adc_t(), &rte_wx_bme280_qf);

		// if modbus RTU is enabled but the quality factor for RTU-pressure is set to NOT_AVALIABLE
		if ((modbus_qf & MODBUS_QF_PRESSURE_FULL) == 0 && (modbus_qf & MODBUS_QF_PRESSURE_DEGR) == 0) {
			// converting raw values to pressure
			bme280_get_pressure(&rte_wx_pressure, bme280_get_adc_p(), &rte_wx_bme280_qf);
		}
		else {
			;
		}

		// if modbus RTU is enabled but the quality factor for RTU-humidity is set to NOT_AVALIABLE
		if ((modbus_qf & MODBUS_QF_HUMIDITY_FULL) == 0 && (modbus_qf & MODBUS_QF_HUMIDITY_DEGR) == 0) {
			// converting raw values to humidity
			bme280_get_humidity(&rte_wx_humidity, bme280_get_adc_h(), &rte_wx_bme280_qf);
		}
		else {
			;	// if the RTU-humidity is set to FULL use that value instead of BME280
		}

		if (rte_wx_bme280_qf == BME280_QF_FULL) {

			// always read the temperature as it is used as an internal temperature in 5th telemetry channel
			rte_wx_temperature_ms_valid = rte_wx_temperature_ms;

			// if modbus RTU is enabled but the quality factor for RTU-humidity is set to non FULL
			if ((modbus_qf & MODBUS_QF_HUMIDITY_FULL) == 0 && (modbus_qf & MODBUS_QF_HUMIDITY_DEGR) == 0) {
				rte_wx_humidity_valid = rte_wx_humidity;
			}
			else {
				;	// if humidity was obtained from RTU sensor use that value and do not bother with BME280
			}

			if ((modbus_qf & MODBUS_QF_PRESSURE_FULL) == 0 && (modbus_qf & MODBUS_QF_PRESSURE_DEGR) == 0) {
				rte_wx_pressure_valid = rte_wx_pressure;

				// add the current pressure into buffer
				rte_wx_pressure_history[rte_wx_pressure_it++] = rte_wx_pressure;

				// reseting the average length iterator
				j = 0;

				// check if and end of the buffer was reached
				if (rte_wx_pressure_it >= PRESSURE_AVERAGE_LN) {
					rte_wx_pressure_it = 0;
				}

				// calculating the average of pressure measuremenets
				for (i = 0; i < PRESSURE_AVERAGE_LN; i++) {

					// skip empty slots in the history to provide proper value even for first wx packet
					if (rte_wx_pressure_history[i] < 10.0f) {
						continue;
					}

					// add to the average
					pressure_average_sum += rte_wx_pressure_history[i];

					// increase the average lenght iterator
					j++;
				}

				rte_wx_pressure_valid = pressure_average_sum / (float)j;
			}
		}
	}
	else {
		// set the quality factor is sensor is not responding on the i2c bus
		rte_wx_bme280_qf = BME280_QF_NOT_AVAILABLE;
	}
#endif

#if (!defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && !defined(_MODBUS_RTU)) || defined (_DALLAS_AS_TELEM) || defined (_MODBUS_RTU) //&& !defined(_RTU_SLAVE_TEMPERATURE_SOURCE)

	rte_wx_temperature_dallas = dallas_query(&rte_wx_current_dallas_qf);

	// checking if communication was successfull
	if (rte_wx_temperature_dallas != -128.0f && ((modbus_qf & MODBUS_QF_TEMPERATURE_FULL) == 0)) {

		// calculate the slew rate
		rte_wx_temperature_dalls_slew_rate = rte_wx_temperature_dallas - rte_wx_temperature_dallas_valid;

		// check the positive slew rate for a hard sensor error
		if (rte_wx_temperature_dalls_slew_rate >  (3 * WX_MAX_TEMPERATURE_SLEW_RATE) && wx_inhibit_slew_rate_check == 0) {
			rte_main_reboot_req = 1;
		}

		// the same but for negative slew
		if (rte_wx_temperature_dalls_slew_rate < (-WX_MAX_TEMPERATURE_SLEW_RATE * 3) && wx_inhibit_slew_rate_check == 0) {
			rte_main_reboot_req = 1;

		}

		// chcecking the positive (ascending) slew rate of the temperature measuremenets
		if (rte_wx_temperature_dalls_slew_rate >  WX_MAX_TEMPERATURE_SLEW_RATE && wx_inhibit_slew_rate_check == 0) {

			// if temeperature measuremenet has changed more than maximum allowed slew rate set degradadet QF
			rte_wx_error_dallas_qf = DALLAS_QF_DEGRADATED;

			// and increase the temperature only by 1.0f to decrease slew rate
			rte_wx_temperature_dallas += 1.0f;

		}

		// chcecking the negaive (descending) slew rate of the temperature measuremenets
		if (rte_wx_temperature_dalls_slew_rate < -WX_MAX_TEMPERATURE_SLEW_RATE && wx_inhibit_slew_rate_check == 0) {

			// if temeperature measuremenet has changed more than maximum allowed slew rate set degradadet QF
			rte_wx_error_dallas_qf = DALLAS_QF_DEGRADATED;

			// and decrease the temperature only by 1.0f to decrease slew rate
			rte_wx_temperature_dallas -= 1.0f;

		}

		// store current value
		rte_wx_temperature_dallas_valid = rte_wx_temperature_dallas;

		// include current temperature into the average
		dallas_average(rte_wx_temperature_dallas, &rte_wx_dallas_average);

		// update the current temperature with current average
		rte_wx_temperature_average_dallas_valid = dallas_get_average(&rte_wx_dallas_average);

		// update current minimal temperature
		rte_wx_temperature_min_dallas_valid = dallas_get_min(&rte_wx_dallas_average);

		// and update maximum also
		rte_wx_temperature_max_dallas_valid = dallas_get_max(&rte_wx_dallas_average);

		// updating last good measurement time
		wx_last_good_temperature_time = master_time;
	}
	else {
		// if there were a communication error set the error to unavaliable
		rte_wx_error_dallas_qf = DALLAS_QF_NOT_AVALIABLE;

	}

	// enabling slew rate checking after first power up
	wx_inhibit_slew_rate_check = 0;
#endif

#if (!defined(_UMB_MASTER) && !defined(_DAVIS_SERIAL) && !defined(_MODBUS_RTU) && defined (_SENSOR_MS5611)) || (defined (_SENSOR_MS5611) && defined (_MODBUS_RTU))
	// quering MS5611 sensor for pressure
	return_value = ms5611_get_pressure(&rte_wx_pressure,  &rte_wx_ms5611_qf);

	if (return_value == MS5611_OK && (modbus_qf & MODBUS_QF_PRESSURE_FULL) == 0 && (modbus_qf & MODBUS_QF_PRESSURE_DEGR) == 0) {
		// add the current pressure into buffer
		rte_wx_pressure_history[rte_wx_pressure_it++] = rte_wx_pressure;

		// reseting the average length iterator
		j = 0;

		// check if and end of the buffer was reached
		if (rte_wx_pressure_it >= PRESSURE_AVERAGE_LN) {
			rte_wx_pressure_it = 0;
		}

		// calculating the average of pressure measuremenets
		for (i = 0; i < PRESSURE_AVERAGE_LN; i++) {

			// skip empty slots in the history to provide proper value even for first wx packet
			if (rte_wx_pressure_history[i] < 10.0f) {
				continue;
			}

			// add to the average
			pressure_average_sum += rte_wx_pressure_history[i];

			// increase the average lenght iterator
			j++;
		}

		rte_wx_pressure_valid = pressure_average_sum / (float)j;
	}

#endif

#if defined(_MODBUS_RTU)

	// unify quality factor across Modbus-RTU sensor and embedded
	// ones.

	// BME280 (or MS5611) has a prioryty over the Modbus-RTU
	if (rte_wx_bme280_qf == BME280_QF_NOT_AVAILABLE ||
		rte_wx_bme280_qf == BME280_QF_UKNOWN)
	{
		// if an internal sensor is not responding or it is not used at all
		// check the result of modbus RTU. this is an a little bit of complicated
		// case as BME280 is a pressure and humidity sensor at once, so changing
		// this QF will also influence the pressure one, but at this point we might
		// agree that we won't use BME280 and external, RTU pressure sensor as it
		// would make no sense to do so
		if ((modbus_qf & MODBUS_QF_HUMIDITY_FULL) > 0) {
			rte_wx_bme280_qf = BME280_QF_FULL;
		}
		else {
			rte_wx_bme280_qf = BME280_QF_UKNOWN;
		}
	}

	// Dallas temperature qualiy factor
	if (rte_wx_error_dallas_qf == DALLAS_QF_NOT_AVALIABLE) {

		// if an internal sensor is not responding check the result of modbus RTU
		if ((modbus_qf & MODBUS_QF_TEMPERATURE_FULL) > 0) {
			rte_wx_error_dallas_qf = DALLAS_QF_UNKNOWN;
			rte_wx_current_dallas_qf = DALLAS_QF_FULL;

		}
		else if ((modbus_qf & MODBUS_QF_TEMPERATURE_DEGR) > 0) {
			rte_wx_error_dallas_qf = DALLAS_QF_DEGRADATED;
			rte_wx_current_dallas_qf = DALLAS_QF_DEGRADATED;
		}
		else if ((modbus_qf & MODBUS_QF_TEMPERATURE_NAVB) > 0) {
			rte_wx_error_dallas_qf = DALLAS_QF_DEGRADATED;
			rte_wx_current_dallas_qf = DALLAS_QF_NOT_AVALIABLE;
		}
	}
#endif

#ifdef _METEO
	// if humidity sensor is idle trigger the communiction & measuremenets
	if (dht22State == DHT22_STATE_DONE || dht22State == DHT22_STATE_TIMEOUT)
		dht22State = DHT22_STATE_IDLE;

#endif


}

void wx_pool_dht22(void) {

	dht22_timeout_keeper();

	switch (dht22State) {
		case DHT22_STATE_IDLE:
			dht22_comm(&rte_wx_dht);
			break;
		case DHT22_STATE_DATA_RDY:
			dht22_decode(&rte_wx_dht);
			break;
		case DHT22_STATE_DATA_DECD:
			rte_wx_dht_valid = rte_wx_dht;			// powrot do stanu DHT22_STATE_IDLE jest w TIM3_IRQHandler
			//rte_wx_dht_valid.qf = DHT22_QF_FULL;
			rte_wx_humidity = rte_wx_dht.humidity;
			dht22State = DHT22_STATE_DONE;
			break;
		case DHT22_STATE_TIMEOUT:
			rte_wx_dht_valid.qf = DHT22_QF_UNAVALIABLE;
			break;
		default: break;
	}

}

void wx_pool_anemometer(void) {

	// locals
	uint32_t average_windspeed = 0;
	int32_t wind_direction_x_avg = 0;
	int32_t wind_direction_y_avg = 0;
	int16_t wind_direction_x = 0;
	int16_t wind_direction_y = 0;
	volatile float dir_temp = 0;
	volatile float arctan_value = 0.0f;
	short i = 0;
	uint8_t average_ln;

#ifdef _MODBUS_RTU
	int32_t modbus_retval;
#endif

	wx_wind_pool_call_counter++;

	uint16_t scaled_windspeed = 0;

	// internal sensors
	#if defined(_ANEMOMETER_ANALOGUE) && !defined(_UMB_MASTER) && !defined(_MODBUS_RTU) || (!defined(_RTU_SLAVE_WIND_DIRECTION_SORUCE) && !defined(_RTU_SLAVE_WIND_SPEED_SOURCE) && defined(_ANEMOMETER_ANALOGUE))
	// this windspeed is scaled * 10. Example: 0.2 meters per second is stored as 2
	scaled_windspeed = analog_anemometer_get_ms_from_pulse(rte_wx_windspeed_pulses);
	#endif

	#if defined(_ANEMOMETER_TX20) && !defined(_UMB_MASTER) && !defined(_MODBUS_RTU) || (!defined(_RTU_SLAVE_WIND_DIRECTION_SORUCE) && !defined(_RTU_SLAVE_WIND_SPEED_SOURCE) && defined(_ANEMOMETER_TX20))
	scaled_windspeed = tx20_get_scaled_windspeed();
	rte_wx_winddirection_last = tx20_get_wind_direction();
	#endif

	#if defined(_UMB_MASTER)
	rte_wx_average_winddirection = umb_get_winddirection();
	rte_wx_average_windspeed = umb_get_windspeed();
	rte_wx_max_windspeed = umb_get_windgusts();
	#else

	#if defined(_MODBUS_RTU) && defined(_RTU_SLAVE_WIND_DIRECTION_SORUCE) && defined(_RTU_SLAVE_WIND_SPEED_SOURCE) && !defined(_RTU_SLAVE_FULL_WIND_DATA)
	// get the value from modbus registers
	modbus_retval = rtu_get_wind_speed(&scaled_windspeed);

	// check if this value has been processed w/o errors
	if (modbus_retval == MODBUS_RET_OK) {
		// if yes continue to further processing
		modbus_retval = rtu_get_wind_direction(&rte_wx_winddirection_last);

	}

	// the second IF to check if the return value was the same for wind direction
	if (modbus_retval == MODBUS_RET_OK || modbus_retval == MODBUS_RET_DEGRADED) {
		// if the value is not available (like modbus is not configured as a source
		// for wind data) get the value from internal sensors..
		#ifdef _INTERNAL_AS_BACKUP
			// .. if they are configured
			scaled_windspeed = analog_anemometer_get_ms_from_pulse(rte_wx_windspeed_pulses);
		#endif
	}
	#elif defined(_MODBUS_RTU) && defined(_RTU_SLAVE_WIND_DIRECTION_SORUCE) && defined(_RTU_SLAVE_WIND_SPEED_SOURCE) && defined(_RTU_SLAVE_FULL_WIND_DATA)
	// get the value from modbus registers
	modbus_retval = rtu_get_wind_direction(&rte_wx_average_winddirection);

	// check if this value has been processed w/o errors
	if (modbus_retval == MODBUS_RET_OK || modbus_retval == MODBUS_RET_DEGRADED) {
		// if yes continue to further processing
		modbus_retval = rtu_get_wind_gusts(&rte_wx_max_windspeed);
		modbus_retval = rtu_get_wind_speed(&rte_wx_winddirection_last);

	}
	#else
	//rte_wx_reset_last_measuremenet_timers(RTE_WX_MEASUREMENT_WIND);
	#endif


#ifndef _RTU_SLAVE_FULL_WIND_DATA
	// check how many times before the pool function was called
	if (wx_wind_pool_call_counter < WIND_AVERAGE_LEN) {
		// if it was called less time than a length of buffers, the average length
		// needs to be shortened to handle the underrun properly
		average_ln = (uint8_t)wx_wind_pool_call_counter;
	}
	else {
		average_ln = WIND_AVERAGE_LEN;
	}

	// putting the wind speed into circular buffer
	rte_wx_windspeed[rte_wx_windspeed_it] = scaled_windspeed;

	// increasing the iterator to the windspeed buffer
	rte_wx_windspeed_it++;

	// checking if iterator reached an end of the buffer
	if (rte_wx_windspeed_it >= WIND_AVERAGE_LEN) {
		rte_wx_windspeed_it = 0;
	}

	// calculating the average windspeed
	for (i = 0; i < average_ln; i++)
		average_windspeed += rte_wx_windspeed[i];

	average_windspeed /= average_ln;

	// store the value in rte
	rte_wx_average_windspeed = average_windspeed;

	// reuse the local variable to find maximum value
	average_windspeed = 0;

	// looking for gusts
	for (i = 0; i < average_ln; i++) {
		if (average_windspeed < rte_wx_windspeed[i])
			average_windspeed = rte_wx_windspeed[i];
	}

	// storing wind gusts value in rte
	rte_wx_max_windspeed = average_windspeed;

	// adding last wind direction to the buffers
	if (rte_wx_winddirection_it >= WIND_AVERAGE_LEN)
		rte_wx_winddirection_it = 0;

	rte_wx_winddirection[rte_wx_winddirection_it++] = rte_wx_winddirection_last;

	// calculating average wind direction
	for (i = 0; i < average_ln; i++) {

		dir_temp = (float)rte_wx_winddirection[i];

		// split the wind direction into x and y component
		wind_direction_x = (int16_t)(100.0f * cosf(dir_temp * direction_constant));
		wind_direction_y = (int16_t)(100.0f * sinf(dir_temp * direction_constant));

		// adding components to calculate average
		wind_direction_x_avg += wind_direction_x;
		wind_direction_y_avg += wind_direction_y;

	}

	// dividing to get average of x and y componen
	wind_direction_x_avg /= average_ln;
	wind_direction_y_avg /= average_ln;

	// converting x & y component of wind direction back to an angle
	arctan_value = atan2f(wind_direction_y_avg , wind_direction_x_avg);

	rte_wx_average_winddirection = (int16_t)(arctan_value * (180.0f/M_PI));

	if (rte_wx_average_winddirection < 0)
		rte_wx_average_winddirection += 360;

#endif

#if defined (_MODBUS_RTU) && (defined(_RTU_SLAVE_WIND_DIRECTION_SORUCE) || defined(_RTU_SLAVE_WIND_SPEED_SOURCE) || defined(_RTU_SLAVE_FULL_WIND_DATA))
	if (modbus_retval == MODBUS_RET_OK) {
		rte_wx_wind_qf = AN_WIND_QF_FULL;
	}
	else if (modbus_retval == MODBUS_RET_DEGRADED) {
		rte_wx_wind_qf = AN_WIND_QF_DEGRADED;
	}
	else if (modbus_retval == MODBUS_RET_NOT_AVALIABLE) {
		rte_wx_wind_qf = AN_WIND_QF_NOT_AVALIABLE;
	}
	else {
		#ifdef _INTERNAL_AS_BACKUP
			rte_wx_wind_qf = analog_anemometer_get_qf();
		#else
			rte_wx_wind_qf = AN_WIND_QF_NOT_AVALIABLE;
		#endif
	}
#elif defined(_ANEMOMETER_ANALOGUE)
	rte_wx_wind_qf = analog_anemometer_get_qf();
#elif defined(_ANEMOMETER_TX20)
	;
#else
	rte_wx_wind_qf = AN_WIND_QF_UNKNOWN;
#endif


	#endif
}

void wx_pwr_init(void) {
	// RELAY_CNTRL
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#if (defined PARATNC_HWREV_A || defined PARATNC_HWREV_B)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
#elif (defined PARATNC_HWREV_C)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#else
#error ("Hardware Revision not chosen.")
#endif
	GPIO_Init(GPIOB, &GPIO_InitStructure);

#if (defined PARATNC_HWREV_C)
	// +12V PWR_CNTRL
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	wx_pwr_state = WX_PWR_OFF;

	GPIO_ResetBits(GPIOB, GPIO_Pin_8);

#if (defined PARATNC_HWREV_C)
	// +12V_SW PWR_CNTRL
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);
#endif

}

void wx_pwr_periodic_handle(void) {

	// do a last valid measuremenets timestamps only if power is currently applied
	if (wx_pwr_state == WX_PWR_ON) {

		// the value of 0xFFFFFFFF is a magic word which disables the check for this parameter
		if (wx_last_good_temperature_time != 0xFFFFFFFF &&
			master_time - wx_last_good_temperature_time >= WX_WATCHDOG_PERIOD)
		{
			wx_pwr_state = WX_PWR_UNDER_RESET;
		}

		// as the weather station could be configured not to perform wind measurements at all
		if (wx_last_good_wind_time != 0xFFFFFFFF &&
			master_time - wx_last_good_wind_time >= WX_WATCHDOG_PERIOD)
		{
			wx_pwr_state = WX_PWR_UNDER_RESET;

			rte_wx_wind_qf = AN_WIND_QF_DEGRADED;
		}

		if (wx_pwr_state == WX_PWR_UNDER_RESET) {
			// if timeout watchod expired there is a time to reset the supply voltage
			wx_pwr_state = WX_PWR_UNDER_RESET;

			// pull the output down to switch the relay and disable +5V_ISOL (VDD_SW)
			GPIO_ResetBits(GPIOB, GPIO_Pin_8);

#ifdef PWR_SWITCH_BOTH
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
#endif

			// setting the last_good timers to current value to prevent reset loop
			wx_last_good_temperature_time = master_time;
			wx_last_good_wind_time = master_time;

			return;
		}

	}

	// service actual supply state
	switch (wx_pwr_state) {
	case WX_PWR_OFF:

		// one second delay
		delay_fixed(2000);

#if (defined PARATNC_HWREV_C)
		// Turn on the +12V_SW voltage
		GPIO_SetBits(GPIOA, GPIO_Pin_6);
#endif

		delay_fixed(100);

		// Turn on the +5V_ISOL (VDD_SW) voltage
		GPIO_SetBits(GPIOB, GPIO_Pin_8);

		// power is off after power-up and needs to be powered on
		wx_pwr_state = WX_PWR_ON;
		break;
	case WX_PWR_ON:
		break;
	case WX_PWR_UNDER_RESET:

		// Turn on the +5V_ISOL (VDD_SW) voltage
		GPIO_SetBits(GPIOB, GPIO_Pin_8);

#ifdef PWR_SWITCH_BOTH
		GPIO_SetBits(GPIOA, GPIO_Pin_6);

		wx_force_i2c_sensor_reset = 1;
#endif

		wx_pwr_state = WX_PWR_ON;

		break;
	case WX_PWR_DISABLED:
		break;
	}
}

//void wx_pwr_disable_12v_sw(void) {
//#if (defined PARATNC_HWREV_C)
//	wx_pwr_state = WX_PWR_DISABLED;
//
//	GPIO_ResetBits(GPIOA, GPIO_Pin_6);
//
//#endif
//}
//
//void wx_pwr_disable_5v_isol(void) {
//	wx_pwr_state = WX_PWR_DISABLED;
//
//	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
//
//
//}
//
//void wx_pwr_enable_12v_sw(void) {
//#if (defined PARATNC_HWREV_C)
//	wx_pwr_state = WX_PWR_OFF;
//
//	// setting last good measurements timers to inhibit relay clicking
//	// just after the power is applied
//	wx_last_good_temperature_time =  master_time;
//	wx_last_good_wind_time = master_time;
//
//#endif
//}
//
//void wx_pwr_enable_5v_isol(void) {
//#if (defined PARATNC_HWREV_C)
//	wx_pwr_state = WX_PWR_OFF;
//
//	// setting last good measurements timers to inhibit relay clicking
//	// just after the power is applied
//	wx_last_good_temperature_time =  master_time;
//	wx_last_good_wind_time = master_time;
//
//#endif
//}
