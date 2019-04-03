/******************************************************************************
* File Name          : GatewayTask.c
* Date First Issued  : 02/25/2019
* Description        : PC<->gateway using usart2, notified by MailboxTask
*******************************************************************************/
/*
  02/26/2019
This task works in conjunction with 'MailboxTask'.  'MailboxTask' notifies this
task when a CAN module circular buffer has one or more CAN msgs.  This task directly
accesses the circular buffer via 'take' pointer.

The 'MailboxTask' is likely a high FreeRTOS priority task.  This task might run at
a lower priority since timing is not critical, however delays require that the 
circular buffer be large enough to avoid overrun since there is no protection for
buffer overflow (since CAN msgs are added to the circular buffer under interrupt).

This version only handles PC->CAN bus msgs for CAN1 module.  To mix CAN1 and CAN2
requires implementing the scheme of commandeering the low order bit(s) from the
sequence number byte.

CAN->PC direction CAN1 and CAN2 msgs are mixed together, except for the cases where
the CANIDs are identical such as DMOC msgs, in which case the CAN2 msgs are tagged 
as 29b address.
*/
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"
#include "GatewayTask.h"
#include "can_iface.h"
#include "MailboxTask.h"
#include "getserialbuf.h"
#include "SerialTaskSend.h"
#include "morse.h"
#include "yprintf.h"
#include "SerialTaskReceive.h"
#include "gateway_PCtoCAN.h"
#include "gateway_CANtoPC.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;

extern struct CAN_CTLBLOCK* pctl1;	// Pointer to CAN1 control block

void StartGatewayTask(void const * argument);

osThreadId GatewayTaskHandle;

/* A notification to Gateway copies the internal notification word to this. */
uint32_t GatewayTask_noteval = 0;    // Receives notification word upon an API notify

/* *************************************************************************
 * osThreadId xGatewayTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: GatewayHandle
 * *************************************************************************/
osThreadId xGatewayTaskCreate(uint32_t taskpriority)
{
 /* definition and creation of CanTask */
  osThreadDef(GatewayTask, StartGatewayTask, osPriorityNormal, 0, 256);
  GatewayTaskHandle = osThreadCreate(osThread(GatewayTask), NULL);
	vTaskPrioritySet( GatewayTaskHandle, taskpriority );

	return GatewayTaskHandle;
}
/* *************************************************************************
 * void StartGatewayTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartGatewayTask(void const * argument)
{

//while(1==1) osDelay(10);

	int i;

	/* The lower order bits are reserved for incoming CAN module msg notifications. */
	#define TSKGATEWAYBITc1	(1 << (STM32MAXCANNUM + 1))  // Task notification bit for huart2 incoming ascii CAN

	/* notification bits processed after a 'Wait. */
	uint32_t noteused = 0;

	/* A notification copies the internal notification word to this. */
	uint32_t noteval = 0;    // Receives notification word upon an API notify

	struct SERIALRCVBCB* prbcb2;	// usart2 (PC->CAN msgs)
	struct CANRCVBUFPLUS* pcanp;  // Basic CAN msg Plus error and seq number
	struct CANRCVBUFN* pncan;

	/* PC-to-CAN msg */
	struct CANTXQMSG testtx;
	testtx.pctl = pctl1;

	/* Setup serial output buffers for uarts. */
	struct SERIALSENDTASKBCB* pbuf2 = getserialbuf(&huart6,128);
	struct SERIALSENDTASKBCB* pbuf3 = getserialbuf(&huart2,128);

	/* Pointers into the CAN  msg circular buffer for each CAN module. */
	struct CANTAKEPTR* ptake[STM32MAXCANNUM] = {NULL};

	/* Setup serial input buffering and line-ready notification */
     //   (ptr uart handle, dma flag, notiification bit, 
     //   ptr notification word, number line buffers, size of lines, 
     //   dma buffer size);
	/* PC-to-CAN ascii/hex incoming "lines" directly converts to CAN msgs. */
	prbcb2 = xSerialTaskRxAdduart(&huart2,1,TSKGATEWAYBITc1,\
		&noteval,12,32,128,1); // buff 12 CAN, of 32 bytes, 192 total dma, /CAN mode
	if (prbcb2 == NULL) morse_trap(41);

	/* Get pointers to circular buffer pointers for each CAN module in list. */	
	for (i = 0; i < STM32MAXCANNUM; i++)
	{
		if ((mbxcannum[i].pmbxarray != NULL) && (mbxcannum[i].pctl != NULL))
		{
			ptake[i] = can_iface_add_take(mbxcannum[i].pctl);
		}
	}

  /* Infinite RTOS Task loop */
  for(;;)
  {
		/* Wait for either PC line completion, or 'MailboxTask' notifications. */
		xTaskNotifyWait(noteused, 0, &GatewayTask_noteval, portMAX_DELAY);
		noteused = 0;	// Accumulate bits in 'noteval' processed.

		/* CAN->PC: Check notification bit for each CAN module */
		for (i = 0; i < STM32MAXCANNUM; i++)
		{
			if ((GatewayTask_noteval & (1 << i)) != 0)
			{
				noteused |= (GatewayTask_noteval & (1 << i)); // We handled the bit
				
				do
				{
					/* Get pointer into CAN msg circular buffer */
					pncan = can_iface_get_CANmsg(ptake[i]);
					if (pncan != NULL)
					{						
						/* Convert binary to the ascii/hex format for PC. */
						xSemaphoreTake(pbuf3->semaphore, 5000);
						gateway_CANtoPC(&pbuf3, &pncan->can);
						vSerialTaskSendQueueBuf(&pbuf3); // Place on queue for usart2 sending
					}
				} while (pncan != NULL);	// Drain the buffer
			}
		}

		/* PC->CAN: Handle incoming usart2 carrying ascii/hex CAN msgs */
		if ((GatewayTask_noteval & TSKGATEWAYBITc1) != 0)
		{ // Here, one or more CAN msgs have been received
			noteused |= TSKGATEWAYBITc1; // We handled the bit

			/* Get incoming CAN msgs from PC and queue for output to CAN bus. */
			do
			{
				pcanp = gateway_PCtoCAN_getCAN(prbcb2);
				if (pcanp != NULL)
				{
					/* Check for errors */
					if (pcanp->error == 0)
					{
						/* Place CAN msg on queue for sending to CAN bus */
						testtx.can = pcanp->can;
						xQueueSendToBack(CanTxQHandle,&testtx,portMAX_DELAY);
					}
					else
					{ // Here, one or more errors. List for the hapless Op to ponder
						yprintf(&pbuf2,"\n\r@@@@@ PC CAN ERROR: %i 0X%04X, 0X%08X 0X%02X 0X%08X %i 0X%02X 0X%02X %s",pcanp->seq, pcanp->error,\
							pcanp->can.id,pcanp->can.dlc,pcanp->can.cd.ui[0]);

						/* For test purposes: Place CAN msg on queue for sending to CAN bus */
						testtx.can = pcanp->can;
						xQueueSendToBack(CanTxQHandle,&testtx,portMAX_DELAY);
					}
				}
			} while ( pcanp != NULL);
		}
  }
}

