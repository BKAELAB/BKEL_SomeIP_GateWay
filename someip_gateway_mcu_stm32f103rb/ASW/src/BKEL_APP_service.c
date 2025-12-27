/*
 * BKEL_UDigno.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_APP_service.h"
#include "BKEL_externs.h"
#include "BKEL_sysconfig.h"

#include "string.h"

ServiceID_t gSid[SERVICE_ID_MAX];
static uint16_t gIdCnt = 0;

/*
 * NOTICE : info text size is must less than 10 !!
 */
BKEL_SERVICE_STATE_T BKEL_CreateServiceID(ServiceID_t* sid)
{
	if (gIdCnt < SERVICE_ID_MAX)
	{
		gSid[gIdCnt++] = *sid;
	}
	else
	{
		return SERVICE_IDCNT_FULL;
	}
	return SERVICE_OK;
}

uint16_t BKEL_GetSIDCount()
{
	return gIdCnt;
}

BKEL_SERVICE_STATE_T BKEL_SendServiceID(void* arg)
{
	BKEL_SERVICE_STATE_T retState;
	char buffer[256];
	int len = 0;

	/*
	 * Packet Frame
	 * DLC(2Byte)|SID(2Byte)|INFO(Dynamic)
	 */
	for (int i = 0 ; i < gIdCnt; ++i)
	{
		uint16_t sid = gSid[i].sid;
		uint16_t dlc = (uint16_t)(2 + strlen(gSid[i].sinfo));
		size_t sinfo_len = strlen(gSid[i].sinfo);

		// 1. set DLC
		memcpy(buffer + len, &dlc, sizeof(uint16_t));
		len += sizeof(uint16_t);

		// 2. copy sid
		memcpy(buffer + len, &sid, sizeof(uint16_t));
		len += sizeof(uint16_t);

		// 3. copy sInfo
		memcpy(buffer + len, gSid[i].sinfo, sinfo_len);
		len += sinfo_len;
	}

	retState = HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
	return (HAL_StatusTypeDef)retState == HAL_OK ? (BKEL_SERVICE_STATE_T)SERVICE_OK : (BKEL_SERVICE_STATE_T)SERVICE_ERROR;
}
