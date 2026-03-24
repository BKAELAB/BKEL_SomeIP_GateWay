/*
 * taskinfo.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "main.h"
#include "stream_buffer.h"
#include "BKEL_APP_protocol.h"
#include "BKEL_APP_rpc.h"
#include "BKEL_APP_sendDiagData.h"

/* Defines */
#define RX_STREAM_SIZE   512

/* LOCAL VARS */
static StaticStreamBuffer_t rxStreamCtrl;
static uint8_t rxStreamStorage[RX_STREAM_SIZE];

static StackType_t sendPeriodAdvertiseStack[BKEL_TASK_STACK_SIZE_MAX];
static StaticTask_t sendPeriodAdvertiseTCB;
static StackType_t handleCommandStack[BKEL_TASK_STACK_SIZE_MAX];
static StaticTask_t handleCommandTCB;
static StackType_t sendDataStack[BKEL_TASK_STACK_SIZE_MIN];
static StaticTask_t sendDataTCB;
static StackType_t RPCStack[BKEL_TASK_STACK_SIZE_MID];
static StaticTask_t RPCTCB;

/* Function Prototypes */
void rtos_taskinit(void);

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

	vTaskDelete(NULL);
	for(;;){
		// Not Use Task (Only Execute 1)
	}
}

BKEL_gpio_pin led;
BKEL_GPIO_STATE_T pinTest;

/* TASK Implementation */
/*
 * Brief : Send to All Service_List for GateWay
 * Period = 5s
 */
void f_sendPeriodAdvertiseTask(void)
{
	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(5000));	// 5s

		AppService_SendAdvertise();
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
        uint32_t notifiedValue;

        xTaskNotifyWait(
            0x00000000,        // clear bits on entry
            0xFFFFFFFF,        // clear bits on exit
            &notifiedValue,
            portMAX_DELAY
        );

/*
 *  #define DIAG_PWM_OUTPUT_VALUE	(0x20U)
	#define DIAG_PWM_INPUT_VALUE	(0x21U)
	#define DIAG_ADC1_GET_VALUE		(0x22U)
	#define DIAG_ADC2_GET_VALUE		(0x23U)
	#define DIAG_GPO_PINSTATE		(0x24U)
	#define DIAG_GPI_PINSTATE		(0x25U)
	#define DIAG_LD2_PINSTATE		(0x26U)
 */
        BKEL_Common_Packet_t *packet = (BKEL_Common_Packet_t *)notifiedValue;
        switch (packet->sid) {
			  case DIAG_PWM_OUTPUT_VALUE: AppSendDiagPWMOut(); break;
			  case DIAG_PWM_INPUT_VALUE: AppSendDiagPWMIn(); break;
			  case DIAG_ADC1_GET_VALUE: AppSendDiagADC1Val(); break;
			  case DIAG_ADC2_GET_VALUE: AppSendDiagADC2Val(); break;
			  case DIAG_GPO_PINSTATE: AppSendDiagGPOPinState(); break;
			  case DIAG_GPI_PINSTATE: AppSendDiagGPIPinState(); break;
			  case DIAG_LD2_PINSTATE: AppSendDiagLD2PinState(); break;
          default: break;
        }
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
        uint32_t notifiedValue;

        xTaskNotifyWait(
            0x00000000,        // clear bits on entry
            0xFFFFFFFF,        // clear bits on exit
            &notifiedValue,
            portMAX_DELAY
        );

        BKEL_Common_Packet_t *packet = (BKEL_Common_Packet_t *)notifiedValue;

        switch ((BKEL_SID_t)packet->sid) {
          case SID_LED_CONTROL: BKEL_RPC_LD2_Control(packet); break;
          case SID_MCU_RESET: BKEL_RPC_MCU_Reset(packet); break;
          case SID_SPI_READ: BKEL_RPC_SPI_Read(packet); break;
          case SID_PWM_SETOUT: BKEL_RPC_PWM_Setout(packet); break;
          default: break;
        }
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
	hSendAdvertiseTask = xTaskCreateStatic(
		(TaskFunction_t)f_sendPeriodAdvertiseTask,
	    "T_Send_Advertise_Period",
		BKEL_TASK_STACK_SIZE_MAX,
	    NULL,
		BKEL_TASK_PRI_NORMAL_0,
		sendPeriodAdvertiseStack,
	    &sendPeriodAdvertiseTCB
	);
	/* Handle Command Task */
	hCommandCustomerTask = xTaskCreateStatic(
		(TaskFunction_t)f_handleCommandTask,
	    "T_Command_Customer",
		BKEL_TASK_STACK_SIZE_MAX,
	    NULL,
		BKEL_TASK_PRI_REALTIME_1,
		handleCommandStack,
	    &handleCommandTCB
	);
	/* Send D_Data Task */
	hSendDataTask = xTaskCreateStatic(
		(TaskFunction_t)f_sendDataTask,
	    "T_Send_Dignostic_Data",
	    BKEL_TASK_STACK_SIZE_MIN,
	    NULL,
	    BKEL_TASK_PRI_REALTIME_2,
	    sendDataStack,
	    &sendDataTCB
	);
	/* RPC Execute Task */
	hRPCTask = xTaskCreateStatic(
		(TaskFunction_t)f_RPCTask,
	    "T_RPC_EXECUTE",
		BKEL_TASK_STACK_SIZE_MID,
	    NULL,
	    BKEL_TASK_PRI_REALTIME_2,
		RPCStack,
	    &RPCTCB
	);
}

