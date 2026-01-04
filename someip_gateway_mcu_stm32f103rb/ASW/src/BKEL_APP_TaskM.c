/*
 * taskinfo.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "main.h"
#include "stream_buffer.h"

/* Defines */
#define RX_STREAM_SIZE   512

/* LOCAL VARS */
static StaticStreamBuffer_t rxStreamCtrl;
static uint8_t rxStreamStorage[RX_STREAM_SIZE];
static StreamBufferHandle_t rxStream;

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
/*
 * Brief : Send to All Service_List for GateWay
 * Period = 5s
 */
void f_sendPeriodAdvertiseTask(void)
{
	for (;;)
	{



#if USE_FEATURE_TEST	// Test Code Here
		AppPwmTest();	// For PWM Test Code.
		AppServiceTest();  // For Packet Send Test.

		/* CRC Test */
		uint8_t test[] = { 0x01, 0x02, 0x03, 0x04 };
		uint8_t crc = calc_crc8(test, sizeof(test));

		/* GPIO DI/DO Test */
		GPIOA->BSRR = (1U << 5);
		GPIOC->BSRR = (1U << 1);
		uint8_t pc0_val = (GPIOC->IDR & 1) == 1 ? 1 : 0;
		uint16_t PC1_VALUE = (GPIOC->IDR & (1 << 1)) ? 1 : 0;
#endif

		vTaskDelay(pdMS_TO_TICKS(5000));	// 5s
	}
}

/*
 * Brief : unBlock Condition = StreamBufferReceive
 * UART Rx ISR : --PUSH--> StreamBuffer , portYIELDFromISR
 * THIS TASK : Frame Parsing , Notify Worker Tasks
 */
void f_handleCommandTask(void)
{
    static uint8_t rx_buf[512];
    static size_t  rx_len = 0;
    uint8_t temp[64];

    for (;;)
    {
        size_t n = xStreamBufferReceive(
            rxStream,
            temp,
            sizeof(temp),
            portMAX_DELAY
        );

        if (n > 0)
        {
            if (rx_len + n > sizeof(rx_buf))
            {
                // overflow check
                rx_len = 0;
                continue;
            }

            memcpy(rx_buf + rx_len, temp, n);
            rx_len += n;

            parse_packet(rx_buf, &rx_len);
        }
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

	rxStream = xStreamBufferCreateStatic(
	        RX_STREAM_SIZE,
	        1,                    // trigger level (1Byte)
	        rxStreamStorage,
	        &rxStreamCtrl
	    );
	configASSERT(rxStream != NULL);

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
			  BKEL_TASK_STACK_SIZE_MAX ,
			  NULL ,
			  BKEL_TASK_PRI_REALTIME_2 ,
			  &hCommandCustomerTask);
	/* Send D_Data Task */
	xTaskCreate((TaskFunction_t)f_sendDataTask ,
			  "T_Send_Dignostic_Data" ,
			  BKEL_TASK_STACK_SIZE_MIN ,
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

