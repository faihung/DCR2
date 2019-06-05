/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#include <string.h>
#include "cantask.h"
#include "flexcan_driver.h"

//#include "tcpecho.h"
//#include "udpecho.h"
#include "tcp_client_server.h"
#include "lwip/opt.h"
#include "lwip/igmp.h"
#include "enetif.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"
/*-----------------------------------------------------------------------------------*/
static char buffer[4]={0x12, 0x34, 0x56, 0x78};//16进制，去掉0x

// structure used to save the pointer to netif passed in udpecho_init
struct netif *netif_shutdown;

static void
//client_task_thread(void *arg)
udpecho_thread(void *arg)
{
	struct netconn *conn;
	struct netconn *conn_igmp;
	struct netbuf *buf;
	err_t err;
    uint8_t i=0,ucQueueMsgValue;
	BaseType_t xResult;
	uint8_t buffer[8] = {0};

	conn = netconn_new(NETCONN_UDP);
	ip_addr_t addr;
	IP_ADDR4(&addr, 192, 168, 0, 221);
    netconn_connect(conn, &addr, 6666);
	LWIP_ERROR("udpecho: invalid conn", (conn != NULL), return;);

	/* create a new netbuf */
	buf = netbuf_new();


	for(;;)
	{
		xResult = xQueueReceive(xQueue_CanNet_record, (void *)&ucQueueMsgValue, pdMS_TO_TICKS(100));
		if(xResult == pdPASS)
		{
			vTaskDelay(10);
			buffer[i++] = ucQueueMsgValue;
			netbuf_ref(buf, buffer, sizeof(buffer));
			if (i == 8)
			{
				i=0;
				err = netconn_send(conn, buf);
				if (err == ERR_OK)
				{
					vTaskDelay(10);
				}
				else
				{
					netconn_delete(conn);
					netbuf_delete(buf);/*回收已经建立的netbuf结构 */
				}
			}
		}

	}



#if THE_PRJ_WAS_THERE
  struct netconn *conn, *newconn;
  err_t err;
  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  /* Bind connection to well known port number 7. */
#if LWIP_IPV6
	conn = netconn_new(NETCONN_TCP_IPV6);
	netconn_bind(conn, IP6_ADDR_ANY, 7);
#else /* LWIP_IPV6 */
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, 7);
#endif /* LWIP_IPV6 */
	LWIP_ERROR("tcpecho: invalid conn", (conn != NULL), return;);

	/* Tell connection to go into listening mode. */
	netconn_listen(conn);


    uint8_t i=0,ucQueueMsgValue;
	BaseType_t xResult;
	uint8_t buff[8] = {0};
	uint8_t bufff[8];
	while(1)
	{
		/* Grab new connection. */
		err = netconn_accept(conn, &newconn);
		vTaskDelay(10);
		if (err == ERR_OK)
		{
			vTaskDelay(10);
			for(;;)
			{
				xResult = xQueueReceive(xQueue_CanNet_record, (void *)&ucQueueMsgValue, pdMS_TO_TICKS(100));
				if(xResult == pdPASS)
				{
					vTaskDelay(10);
					buff[i++] = ucQueueMsgValue;
					if (i == 8)
					{
						i=0;
						netconn_write(newconn, buff, 8, NETCONN_COPY);
					}
				}else{
					//netbuf_delete(newconn);
					//break;
				}
			}
			/* Close connection and discard connection identifier. */
			netconn_close(newconn);
			netconn_delete(newconn);
		}else{
			printf("Can not create TCP netconn.\r\n");
			vTaskDelay(10);
		}
	}
#endif
}

static void
server_task_thread(void *arg)
{
	struct netconn *conn, *newconn;
	err_t err;
	LWIP_UNUSED_ARG(arg);
	MSG_CMD *ptMsg;
	ptMsg = &g_tMsgCmd;

	/* Create a new connection identifier. */
	/* Bind connection to well known port number 7. */
	#if LWIP_IPV6
	conn = netconn_new(NETCONN_TCP_IPV6);
	netconn_bind(conn, IP6_ADDR_ANY, 8);
	#else /* LWIP_IPV6 */
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, 7);
	#endif /* LWIP_IPV6 */
	LWIP_ERROR("tcpecho: invalid conn", (conn != NULL), return;);

	/* Tell connection to go into listening mode. */
	netconn_listen(conn);

#if 1 //THE_PRJ_WAS_THERE
	while (1) {
		/* Grab new connection. */
		err = netconn_accept(conn, &newconn);
		/* Process the new connection. */
		if (err == ERR_OK) {//连接...
		  struct netbuf *buf;
		  void *dataCmd;
		  u16_t len;
		  char data[64];
		  char *ptoken;
		  char *delim = ",";


		  while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
			do {
				 netbuf_data(buf, &dataCmd, &len);
				 err = netconn_write(newconn, dataCmd, len, NETCONN_COPY);
			} while (netbuf_next(buf) >= 0);
			netbuf_delete(buf);

		#if DEBUG_TEST_OPEN
			memset(data,0,sizeof(data));
			memcpy(data,dataCmd,len);
			ptoken = strtok(data,delim);
			memcpy(ptMsg->OnOff,ptoken,2);
			if(ptoken != NULL)
			{
			   ptoken = strtok(NULL, delim);
			   memcpy(ptMsg->Select,ptoken,8);
			}
			if(ptoken != NULL)
			{
			   ptoken = strtok(NULL, delim);
			   memcpy(ptMsg->AddFilter,ptoken,3);
			}
			xQueueSendToBack(xQueue_CanNet_config, (void *)&ptMsg, 10);
		#endif
		  }

		  /* Close connection and discard connection identifier. */
		  netconn_close(newconn);
		  netconn_delete(newconn);
		}
	}
#endif
}

/*-----------------------------------------------------------------------------------*/
void
tcp_client_server_init(void)
{
	//sys_thread_new("tcpecho_thread", client_task_thread, NULL, 1024, DEFAULT_THREAD_PRIO);
	sys_thread_new("tcpecho_thread", server_task_thread, NULL, 1024, DEFAULT_THREAD_PRIO);
}

void
udp_client_server_init(struct netif *netif)
{
	netif_shutdown = netif;
	sys_thread_new("udpecho_thread", udpecho_thread, NULL, 1024, DEFAULT_THREAD_PRIO);
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
