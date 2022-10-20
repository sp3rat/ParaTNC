/*
 * max31865.h
 *
 *  Created on: Sep 25, 2022
 *      Author: mateusz
 */

#ifndef INCLUDE_DRIVERS_MAX31865_H_
#define INCLUDE_DRIVERS_MAX31865_H_

#include "drivers/spi.h"

#include <stdint.h>

#define MAX_3WIRE	3
#define MAX_4WIRE	1

typedef enum max31865_qf_t {
	MAX_QF_FULL
}max31865_qf_t;

void max31865_init(uint8_t rdt_type, uint8_t reference_resistor);
void max31865_pool(void);
int32_t max31865_get_pt100_result(max31865_qf_t * quality_factor);
int32_t max31865_get_result(uint32_t RTDnominal);


#endif /* INCLUDE_DRIVERS_MAX31865_H_ */
