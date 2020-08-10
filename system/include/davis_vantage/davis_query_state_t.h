/*
 * davis_query_state_t.h
 *
 *  Created on: 10.08.2020
 *      Author: mateusz
 */

#ifndef INCLUDE_DAVIS_VANTAGE_DAVIS_QUERY_STATE_T_H_
#define INCLUDE_DAVIS_VANTAGE_DAVIS_QUERY_STATE_T_H_

typedef enum davis_query_state {
	DAVIS_QUERY_IDLE,
	DAVIS_QUERY_SENDING_QUERY,
	DAVIS_QUERY_RECEIVING_ACK,
	DAVIS_QUERY_RECEIVING,
	DAVIS_QUERY_OK,
	DAVIS_QUERY_ERROR
}davis_query_state_t;

#endif /* INCLUDE_DAVIS_VANTAGE_DAVIS_QUERY_STATE_T_H_ */
