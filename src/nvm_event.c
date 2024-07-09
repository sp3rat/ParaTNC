#include "nvm_event.h"
#include "nvm_configuration.h"
#include "memory_map.h"
#include "backup_registers.h"

/**
 * 
*/
#define NVM_EVENT_CREATE_POINTERS_FOR_TARGET(_name, _non_ptr_based_write_function, _area_start_addr, _area_end_addr, _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity, pointer_based_access) \
		event_log_t** nvm_event_oldest##_name;							\
		event_log_t** nvm_event_newest##_name;							\		
																		\

/**
 *
 */
#define NVM_EVENT_PUSH_POINTERS_ARITM(_area_start_addr, _area_end_addr, _erase_fn, _severity)	\
if (EVENT_LOG_GET_SEVERITY(event->severity_and_source) >= _severity )	{						\
	/* check if we reach boundary between two flash memory pages */								\
	/* and the newest entry is just before the oldest pne  */									\
	if (next_newest_init_ptr == oldest_init_ptr) {												\
		/* erase next flash memory page to make a room for next events   */						\
		flash_status = _erase_fn (*oldest);														\
																								\
		/* check operation result */															\
		if (flash_status != FLASH_COMPLETE) {													\
			nvm_general_state = NVM_PGM_ERROR;													\
		}																						\
																								\
		/* rescan for oldest and newest event one more time  */									\
		nvm_event_log_find_first_oldest_newest(oldest, newest, (void*)_area_start_addr, (void*)_area_end_addr);	\
																								\
		const uint8_t old_new_events_spacing = *oldest - *newest;								\
																								\
		/* oldest - newest should be located NVM_PAGE_SIZE bytes apart  */						\
		/* please note, that pointers points to the beginning of each  */						\
		/* entry, hence this minus one  */														\
		if ((old_new_events_spacing - 1) * sizeof(event_log_t) != NVM_PAGE_SIZE) {				\
			backup_assert(BACKUP_REG_ASSERT_ERASE_FAIL_WHILE_STORING_EVENT);					\
		}																						\
																								\
		/* move pointer to newest, to point to a place where  */								\
		/* newly inserted event will be located  */												\
		*newest = *(newest) + 1;																\
	}  																							\
	else if ((void*)next_newest_init_ptr >= (void*)_area_end_addr) {							\
		/* we have reached an end of the event area in flash  */								\
																								\
		/* erase first memory page  */															\
		flash_status = _erase_fn (_area_start_addr);											\
																								\
		/* set pointers accordingly  */															\
		event_log_t* new_newest = (event_log_t*)_area_start_addr;								\
		event_log_t* new_oldest = (event_log_t*)(_area_end_addr - sizeof(event_log_t));			\
																								\
		*newest = new_newest;																	\
		*oldest = new_oldest;																	\
	}																							\
	else {																						\
		*newest = *(newest) + 1;																\
	}																							\
}																								\

/**
 *
 */
#define NVM_EVENT_PUSH_POINTERS_FLASH_OPER(_event_to_insert, _area_end_addr, _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity)	\
if (EVENT_LOG_GET_SEVERITY(event->severity_and_source) >= _severity )	{																		\
	/* programming 32 bits at once */																											\
	uint32_t * ptr_event_to_insert = (uint32_t*)_event_to_insert;																				\
	uint32_t * ptr_place_for_new_event = (uint32_t*)*newest;																					\
																																				\
	_enable_pgm_fn																																\
																																				\
	*((uint32_t*)(ptr_place_for_new_event)) = *ptr_event_to_insert;																				\
	_wait_for_pgm_fn																															\
																																				\
	*((uint32_t*)(ptr_place_for_new_event)+ 1) = *(ptr_event_to_insert + 1);																	\
	_wait_for_pgm_fn																															\
																																				\
	*((uint32_t*)(ptr_place_for_new_event)+ 2) = *(ptr_event_to_insert + 2);																	\
	_wait_for_pgm_fn																															\
																																				\
	*((uint32_t*)(ptr_place_for_new_event)+ 3) = *(ptr_event_to_insert + 3);																	\
	_wait_for_pgm_fn																															\
																																				\
	_disable_pgm_fn																																\
																																				\
}																																				\

/**
 *
 */
#define NVM_EVENT_PUSH_POINTERS_ARITM_SEC(_area_start_addr, _area_end_addr)														\
	/* rescan for oldest and newest event one more time */																		\
	nvm_event_log_find_first_oldest_newest(oldest, newest, (void*)_area_start_addr, (void*)_area_end_addr);						\
																																\

/**
 *
 */
#define NVM_EVENT_EXPAND_POINTER_BASE_ACCESS_true(_area_start_addr, _area_end_addr, _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity)		\
	NVM_EVENT_PUSH_POINTERS_ARITM(_area_start_addr, _area_end_addr, _erase_fn, _severity);																			\
	NVM_EVENT_PUSH_POINTERS_FLASH_OPER(event, _area_end_addr,  _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity);							\
	NVM_EVENT_PUSH_POINTERS_ARITM_SEC(_area_start_addr, _area_end_addr);																							\

/**
 *
 */
#define NVM_EVENT_EXPAND_POINTER_BASE_ACCESS(_name, _non_ptr_based_write_function, _area_start_addr, _area_end_addr, _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity, pointer_based_access)	\
	NVM_EVENT_EXPAND_POINTER_BASE_ACCESS_##pointer_based_access(_area_start_addr, _area_end_addr, _erase_fn, _enable_pgm_fn, _wait_for_pgm_fn, _disable_pgm_fn, _severity);				\


static nvm_state_result_t nvm_general_state = NVM_UNINITIALIZED;


NVM_EVENT_LOGGING_TARGETS(NVM_EVENT_CREATE_POINTERS_FOR_TARGET);

/**
 *
 * @param oldest
 * @param newest
 */
nvm_event_result_t nvm_event_log_find_first_oldest_newest(event_log_t** oldest, event_log_t** newest, void * area_start, void * area_end) {

	nvm_event_result_t res = NVM_EVENT_OK;

	// pointer to last, non null and non TIMESYNC entry
	event_log_t* last_non_ts = NULL;

	// pointer to the oldest non TIMESYNC event log entry
	event_log_t* oldest_non_ts = NULL;

	// size of single log entry
	const uint8_t log_entry_size = sizeof(event_log_t);

	// how any events could be stored in NVM flash memory
	const uint16_t log_entries = (area_end - area_start) / log_entry_size;

	// lowest date found within events in NVM
	uint32_t lowest_date = 0xFFFFFFFFu;

	uint32_t lowest_time = 0xFFFFFFFFu;

	// sanity check if everything is set correctly
	if ((area_end - area_start) % log_entry_size != 0 ) {
		return NVM_EVENT_AREA_ERROR;
	}

	last_non_ts = 		(event_log_t *)area_start;

	// iterate through all event log flash area
	for (int i = 0; i < log_entries; i++) {

		// set pointer to currently checked event
		const event_log_t* const current = (area_start + (log_entry_size) * i);

		event_log_severity_t severity = (current->severity_and_source & 0xF0) >> 4;
		event_log_source_t source = (current->severity_and_source & 0xF);

		// skip erased memory
		if (current->event_id == 0xFFU && current->event_master_time == 0xFFFFFFFFU) {
			oldest_non_ts = NULL;
			continue;
		}

		// look for timesync event created at bootup
		if (current->event_id == EVENT_TIMESYNC && current->wparam == 0x77) {

			// check if this timestamp is before the oldest found before
			if (lowest_date > current->lparam && lowest_time > current->lparam2) {

				// set this as the oldest
				lowest_date = current->lparam;
				lowest_time = current->lparam2;

				// timestamp are always created after the first one after power up, so that
				// with oldest RTC date and time will be the oldest in general
				*oldest = (event_log_t*)current;
			}
		}
		else {
			if (current->event_master_time > last_non_ts->event_master_time) {
				// store a pointer to last non-null and non-timesync event
				last_non_ts = (event_log_t*)current;

				// updated output pointer with newest 
				*newest = last_non_ts;
			}
			else {
				// this loop goes forward in memory. if consecutive non timesync event
				// has decreasing master_time value it means, that nvm events area 
				// has overruned at least one time 
				res = NVM_EVENT_OVERRUN;

				if (oldest_non_ts == NULL) {
					oldest_non_ts = (event_log_t*)current;
				}
			}
		}
	}

	// check if any non-timesync event has been found at all
	if (last_non_ts == NULL) {
		// no, NVM log contains only single timesync event
		res = NVM_EVENT_SINGLE_TS;
	}

	// check if any timesync event has been found
	if (lowest_date == 0xFFFFFFFFu && lowest_time == 0xFFFFFFFF) {
		if (last_non_ts == (event_log_t *)area_start) {
			res = NVM_EVENT_EMPTY;	// nvm event area is empty
		}
		else {
			*oldest = oldest_non_ts;
			res = NVM_EVENT_OVERRUN_NO_TS;
		}
	}

	return res;

}

/**
 * @param event
 * @param oldest
 * @param newest
 */
nvm_event_result_t nvm_event_log_push_new_event(event_log_t* event, event_log_t** oldest, event_log_t** newest) {
	nvm_event_result_t out = NVM_EVENT_OK;

	// flash operation result
	FLASH_Status flash_status = 0;

	// pointers
	const event_log_t* oldest_init_ptr 			= *oldest;
	const event_log_t* next_newest_init_ptr 	= *newest + 1;

	NVM_EVENT_LOGGING_TARGETS(NVM_EVENT_EXPAND_POINTER_BASE_ACCESS);


	return out;
}
