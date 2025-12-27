/*
 * taskinfo.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "main.h"

/* Defines */
#define QUEUE_LENGTH 10
#define ITEM_SIZE    sizeof(ServiceID_t)

/* LOCAL VARS */
static StaticQueue_t xQueueBuffer;
static ServiceID_t ucQueueStorage[ QUEUE_LENGTH * ITEM_SIZE ];

/* Function Prototypes */
void rtos_taskinit(void);
void app_serviceInit(void);

// Tasks
void f_sendPeriodAdvertiseTask(void);
void f_handleCommandTask(void);
void f_sendDataTask(void);
void f_RPCTask(void);

/* Definitions for initTask */
void f_inittask(void)
{
	// Task Init
	rtos_taskinit();

	// APP Init
	app_serviceInit();

	vTaskDelete(NULL);
	for(;;){
		// Not Use Task (Only Execute 1)
	}
}

/* TASK Implementation */
void f_sendPeriodAdvertiseTask(void)
{
	for (;;)
	{
		/* TO DO
		 * advertise ALL Service ID & Infomation
		 * Period = 5s
		 */


		vTaskDelay(pdMS_TO_TICKS(5000));	// 5s
	}
}

void f_handleCommandTask(void)
{
	ServiceID_t sid;
	for(;;)
	{
		/*
		 * TO DO
		 * UART Rx ISR(DMA/IDLE) -> PushQueue
		 * DLC|SID|OPCODE
		 * Parsing sid & opcode
		 * branch sid (RPC or Diagnostic Data)
		 */
		xQueueReceive(hQueue, &sid, portMAX_DELAY); // Block until push data to Queue


	}
}

void f_sendDataTask(void)
{
	for(;;)
	{
		/*
		 * TO DO
		 * Send Diagnostic Data
		 * Example : GPIO Pin State or ADC Value
		 */
		uint16_t command;

		command = ulTaskNotifyTake(pdFALSE,
						 	 	   portMAX_DELAY); // Block until xTaskNotifyGive();
	}
}

void f_RPCTask(void)
{
	for(;;)
	{
		/*
		 * TO DO
		 * Remote Procedure Call Task
		 * Example : Toggle LED2
		 */
		uint16_t command;

		command = ulTaskNotifyTake(pdFALSE,
						 	 	   portMAX_DELAY); // Block until xTaskNotifyGive();
	}
}


/* RTOS TASK INIT */
void rtos_taskinit(void)
{

	hQueue = xQueueGenericCreateStatic(QUEUE_LENGTH,
									   ITEM_SIZE,
									   ucQueueStorage,
									   &xQueueBuffer, 0 );

	/* Advertise Task */
	xTaskCreate((TaskFunction_t)f_sendPeriodAdvertiseTask ,
			  "T_Send_Advertise_Period" ,
			  BKEL_TASK_STACK_SIZE_MAX ,
			  NULL ,
			  BKEL_TASK_PRI_NORMAL_0,
			  &hSendAdvertiseTask	 );
	/* Handle Command Task */
	xTaskCreate((TaskFunction_t)f_handleCommandTask ,
			  "T_Command_Customer" ,
			  BKEL_TASK_STACK_SIZE_MIN ,
			  NULL ,
			  BKEL_TASK_PRI_REALTIME_2 ,
			  &hCommandCustomerTask);
	/* Send D_Data Task */
	xTaskCreate((TaskFunction_t)f_sendDataTask ,
			  "T_Send_Dignostic_Data" ,
			  BKEL_TASK_STACK_SIZE_MID ,
			  NULL ,
			  BKEL_TASK_PRI_REALTIME_1 ,
			  &hSendDataTask);
	/* RPC Execute Task */
	xTaskCreate((TaskFunction_t)f_RPCTask ,
			  "T_RPC_EXECUTE" ,
			  BKEL_TASK_STACK_SIZE_MID ,
			  NULL ,
			  BKEL_TASK_PRI_REALTIME_1 ,
			  &hRPCTask);
}

void app_serviceInit(void)
{

}

