/*
 * memory_map.h
 *
 *  Created on: May 26, 2024
 *      Author: mateusz
 */

#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

#define MEMORY_MAP_BOOTLOADER_START				0x08000800
#define MEMORY_MAP_BOOLOADER_END				0x0800BFFF

#define MEMORY_MAP_APPLICATION_START			0x0800C000
#define MEMORY_MAP_APPLICATION_END				0x0803C7FF

#define MEMORY_MAP_CONFIG_SECTION_FIRST_START	0x0803F000
#define MEMORY_MAP_CONFIG_SECTION_FIRST_END		0x0803F7FF

#define MEMORY_MAP_CONFIG_SECTION_SECOND_START	0x0803F800
#define MEMORY_MAP_CONFIG_SECTION_SECOND_END	0x0803FFFF

#define MEMORY_MAP_CONFIG_SECTION_DEFAULT_START 0x08040000
#define MEMORY_MAP_CONFIG_SECTION_DEFAULT_END 	0x080407FF

#define MEMORY_MAP_CONFIG_END					0x080407FF

#define MEMORY_MAP_MEASUREMENT_512K_START		0x08041000
#define MEMORY_MAP_MEASUREMENT_1M_START			0x08080000

#define MEMORY_MAP_MEASUREMENT_512K_PAGES		48
#define MEMORY_MAP_MEASUREMENT_1M_PAGES			96

#define MEMORY_MAP_EVENT_LOG_START				0x0805A800
#define MEMORY_MAP_EVENT_LOG_END				0x0807CFFF

#define MEMORY_MAP_FLASH_END	FLASH_BANK1_END

#endif /* MEMORY_MAP_H_ */
