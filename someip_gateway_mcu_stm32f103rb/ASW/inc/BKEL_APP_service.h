/*
 * BKEL_UDigno.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_UDIGNO_H_
#define INC_BKEL_UDIGNO_H_
#include "BKEL_typedef.h"

typedef enum BKEL_SERVICE_STATE {
	SERVICE_OK = 0,
	SERVICE_IDCNT_FULL,
	SERVICE_ERROR,
}BKEL_SERVICE_STATE_T;

typedef void (*serviceFunc)(void*);
typedef struct ServiceID {
	uint16_t sid;
	serviceFunc scb;
	const char* sinfo;
}ServiceID_t;

BKEL_SERVICE_STATE_T BKEL_CreateServiceID(ServiceID_t* sid);
uint16_t BKEL_GetSIDCount();
BKEL_SERVICE_STATE_T BKEL_SendServiceID();


#endif /* INC_BKEL_UDIGNO_H_ */
