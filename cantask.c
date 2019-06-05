/* ###################################################################
**     Filename    : main.c
**     Project     : flexcan_mpc5748g
**     Processor   : MPC5748G_176
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2017-11-06, 17:35, # CodeGen: 2
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.00
** @brief
**         Main module.
**         This module contains user's application code.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/

/* MODULE main */
#include "string.h"
#include "cantask.h"
#include "Cpu.h"
#include "clockMan1.h"
#include "canCom1.h"
#include "dmaController1.h"
#include "pin_mux.h"
#if CPU_INIT_CONFIG
  #include "Init_Config.h"
#endif




uint8_t ledRequested = (uint8_t)LED0_CHANGE_REQUESTED;
/******************************************************************************
 * Functions
 ******************************************************************************/

/**
 * Button interrupt handler
 */
void buttonISR(void)
{
    /* Check if one of the buttons was pressed */
    uint32_t button0 = PINS_DRV_GetPinExIntFlag(BTN0_EIRQ);
    uint32_t button1 = PINS_DRV_GetPinExIntFlag(BTN1_EIRQ);

    bool sendFrame = false;

    /* Set FlexCAN TX value according to the button pressed */
    if (button0 != 0)
    {
        ledRequested = LED0_CHANGE_REQUESTED;
        sendFrame = true;
        /* Clear interrupt flag */
        PINS_DRV_ClearPinExIntFlag(BTN0_EIRQ);
    }
    else if (button1 != 0)
    {
        ledRequested = LED1_CHANGE_REQUESTED;
        sendFrame = true;
        /* Clear interrupt flag */
        PINS_DRV_ClearPinExIntFlag(BTN1_EIRQ);
    }
    else
    {
        PINS_DRV_ClearExIntFlag();
    }

    if (sendFrame)
    {
        /* Send the information via CAN */
        SendCANData(TX_MAILBOX, TX_MSG_ID, &ledRequested, 1UL);
    }
}

/*
 * @brief: Send data via CAN to the specified mailbox with the specified message id
 * @param mailbox   : Destination mailbox number
 * @param messageId : Message ID
 * @param data      : Pointer to the TX data
 * @param len       : Length of the TX data
 * @return          : None
 */
void SendCANData(uint32_t mailbox, uint32_t messageId, uint8_t * data, uint32_t len)
{
    /* Set information about the data to be sent
     *  - 1 byte in length
     *  - Standard message ID
     *  - Bit rate switch enabled to use a different bitrate for the data segment
     *  - Flexible data rate enabled
     *  - Use zeros for FD padding
     */
    flexcan_data_info_t dataInfo =
    {
            .data_length = len,
            .msg_id_type = FLEXCAN_MSG_ID_STD,
            .enable_brs  = false,
            .fd_enable   = false,
            .fd_padding  = 0U
    };

    /* Configure TX message buffer with index TX_MSG_ID and TX_MAILBOX*/
    FLEXCAN_DRV_ConfigTxMb(INST_CANCOM1, mailbox, &dataInfo, messageId);

    /* Execute send non-blocking */
    FLEXCAN_DRV_Send(INST_CANCOM1, mailbox, &dataInfo, messageId, data);
}

/*
 * @brief Function which configures the LEDs and Buttons
 */
void GPIOInit(void)
{
    /* Set Output value LEDs */
    PINS_DRV_ClearPins(LED_PORT, (1 << LED0) | (1 << LED1));

    SIUL2->IMCR[155] = SIUL2_IMCR_SSS(1U);
    SIUL2->IMCR[144] = SIUL2_IMCR_SSS(1U);

    /* Install buttons ISR */
    INT_SYS_InstallHandler(SIUL_EIRQ_00_07_IRQn, &buttonISR, NULL);
    INT_SYS_InstallHandler(SIUL_EIRQ_08_15_IRQn, &buttonISR, NULL);

    /* Enable buttons interrupt */
    INT_SYS_EnableIRQ(SIUL_EIRQ_00_07_IRQn);
    INT_SYS_EnableIRQ(SIUL_EIRQ_08_15_IRQn);
}


void canDataTask(void *pvParameters)
{
	uint8_t i;
	MSG_CMD *ptMsg;
	const char* pStart = "On";

#if 1
	/* Do the initializations required for this application */
	GPIOInit();

	/*
	 * Initialize FlexCAN driver
	 *  - 8 byte payload size
	 *  - FD enabled
	 *  - Bus clock as peripheral engine clock
	 */
	FLEXCAN_DRV_Init(INST_CANCOM1, &canCom1_State, &canCom1_InitConfig0);

	/* Set information about the data to be received
	 *  - 1 byte in length
	 *  - Standard message ID
	 *  - Bit rate switch enabled to use a different bitrate for the data segment
	 *  - Flexible data rate enabled
	 *  - Use zeros for FD padding
	 */
	flexcan_data_info_t dataInfo =
	{
		.data_length = 8U,
		.msg_id_type = FLEXCAN_MSG_ID_STD,
		.enable_brs  = false,
		.fd_enable   = false,
		.fd_padding  = 0U
	};

	/* Configure RX message buffer with index RX_MSG_ID and RX_MAILBOX */
	FLEXCAN_DRV_ConfigRxMb(INST_CANCOM1, RX_MAILBOX, &dataInfo, RX_MSG_ID);
	FLEXCAN_DRV_ConfigTxMb(INST_CANCOM1, TX_MAILBOX, &dataInfo, TX_MSG_ID);


	while(1)
	{
		xQueueReceive(xQueue_CanNet_config, (void *)&ptMsg, pdMS_TO_TICKS(100));

		/* Define receive buffer */
		flexcan_msgbuff_t recvBuff;
		if(0 == memcmp(ptMsg->OnOff,pStart,2))
		{
			/* Start receiving data in RX_MAILBOX. */
			FLEXCAN_DRV_Receive(INST_CANCOM1, RX_MAILBOX, &recvBuff);
			//receive数据之后，在这里添加过滤规则

			FLEXCAN_DRV_Send(INST_CANCOM1,TX_MAILBOX, &dataInfo, 0x777, (const uint8_t*)&recvBuff.data);
			for (i=0; i<8; i++)
			{
				xQueueSendToBack(xQueue_CanNet_record, &recvBuff.data[i], 10);
			}

			/* Wait until the previous FlexCAN receive is completed */
			while(FLEXCAN_DRV_GetTransferStatus(INST_CANCOM1, RX_MAILBOX) == STATUS_BUSY);

			/* Toggle output value LED1 */
			PINS_DRV_TogglePins(LED_PORT, (1 << LED0));

//			/* Check the received message ID and payload */
//			if((recvBuff.data[0] == LED0_CHANGE_REQUESTED) &&
//					recvBuff.msgId == RX_MSG_ID)
//			{
//				/* Toggle output value LED1 */
//				PINS_DRV_TogglePins(LED_PORT, (1 << LED0));
//			}
//			else if((recvBuff.data[0] == LED1_CHANGE_REQUESTED) &&
//					recvBuff.msgId == RX_MSG_ID)
//			{
//				/* Toggle output value LED0 */
//				PINS_DRV_TogglePins(LED_PORT, (1 << LED1));
//			}
		}

	}
#endif
}


