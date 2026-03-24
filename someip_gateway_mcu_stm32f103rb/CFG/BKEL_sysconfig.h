/*
 * systemconfig.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_SYSCONFIG_H_
#define INC_BKEL_SYSCONFIG_H_
#include <BKEL_typedef.h>

/* For SOME/UART Config */
#define USE_UART_DEBUG			1U
#define SERVICE_ID_MAX			10U

/* FreeRTOS */
// stack size
#define BKEL_TASK_STACK_SIZE_MIN	((uI16_T)128)
#define BKEL_TASK_STACK_SIZE_MID	((uI16_T)192)
#define BKEL_TASK_STACK_SIZE_MAX	((uI16_T)256)

// Priority
#define BKEL_TASK_PRI_LOW_0	 		2U + 0
#define BKEL_TASK_PRI_LOW_1	 		2U + 1U
#define BKEL_TASK_PRI_LOW_2	 		2U + 2U

#define BKEL_TASK_PRI_NORMAL_0 		5U + 0
#define BKEL_TASK_PRI_NORMAL_1 		5U + 1U
#define BKEL_TASK_PRI_NORMAL_2 		5U + 2U

#define BKEL_TASK_PRI_REALTIME_0 	10U + 0
#define BKEL_TASK_PRI_REALTIME_1 	10U + 1U
#define BKEL_TASK_PRI_REALTIME_2 	10U + 2U

#endif /* INC_BKEL_SYSCONFIG_H_ */
