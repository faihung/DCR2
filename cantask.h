#ifndef __CAN_TASK_H__
#define __CAN_TASK_H__
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include <stdint.h>
#include <stdbool.h>
/******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEBUG_TEST_OPEN		1U
#define DEBUG_TEST_CLOSE	0U
#define THE_PRJ_WAS_THERE	0U//工程原本就有的

#define LED_PORT        PTA
#define LED0            10U
#define LED1             7U

#define BTN0_PORT       PTE
#define BTN1_PORT       PTA
#define BTN0_PIN        12U
#define BTN1_PIN         3U
#define BTN0_EIRQ       11U
#define BTN1_EIRQ        0U

/* Use this define to specify if the application runs as master or slave */
#define MASTER
/* #define SLAVE */

/* Definition of the TX and RX message buffers depending on the bus role */
#if defined(MASTER)
    #define TX_MAILBOX  (1UL)
    #define TX_MSG_ID   (1UL)
    #define RX_MAILBOX  (0UL)
    #define RX_MSG_ID   (2UL)//(2UL)
#elif defined(SLAVE)
    #define TX_MAILBOX  (0UL)
    #define TX_MSG_ID   (2UL)
    #define RX_MAILBOX  (1UL)
    #define RX_MSG_ID   (1UL)
#endif

typedef enum
{
    LED0_CHANGE_REQUESTED = 0x00U,
    LED1_CHANGE_REQUESTED = 0x01U
} can_commands_list;

typedef struct Msg
{
	uint8_t OnOff[4];
	uint8_t Select[8];
	uint8_t AddFilter[32];
}MSG_CMD;
MSG_CMD   g_tMsgCmd; /*定义一个结构体用于消息队列 */

QueueHandle_t xQueue_CanNet_record;
QueueHandle_t xQueue_CanNet_config;

/******************************************************************************
 * Function prototypes
 ******************************************************************************/
void SendCANData(uint32_t mailbox, uint32_t messageId, uint8_t * data, uint32_t len);
void buttonISR(void);
void GPIOInit(void);
void canDataTask(void *pvParameters);
void DataCollectRecord(MSG_CMD *PtMsg);
#endif
