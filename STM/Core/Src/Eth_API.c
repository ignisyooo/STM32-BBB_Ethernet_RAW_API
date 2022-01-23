/*
 * Eth_API.c
 *
 *  Created on: 23 lis 2021
 *      Author: rzeszutko
 */

#ifndef SRC_ETH_API_C_
#define SRC_ETH_API_C_

#include "Eth_API.h"

#include <string.h>

extern ETH_HandleTypeDef heth;
extern struct netif gnetif;

static const eth_address source = { 0x00, 0x80, 0xE2, 0x00, 0x00, 0x00};

static uint8_t receive_bytes[MAX_DATA_SIZE];

/****************************************************************************************************************************************
 * Local functions
 ***************************************************************************************************************************************/
static uint16_t RevertEndianness(uint16_t data);
static void ETH_input_procces(void);
static void ethernetif_notify_conn_changed(struct netif *netif);
static void Ethernet_Read_Data(struct pbuf *p);
static struct pbuf * Eth_CreateBuffer(struct netif* net, const eth_address src_mac, const eth_address dst_mac, const EthernetPayload_T* payload);
static void ETH_ErrorHandle(void);
static bool low_level_Tranmission(struct pbuf *p);
static struct pbuf * get_data_buffer(void);

/* Main function to process input data and check connection status */
void ETH_Proccess(void)
{
	/* check input data */
	ETH_input_procces();

	/* Get link up  */
	ethernetif_set_link(&gnetif);

	if(netif_is_link_up(&gnetif) && !netif_is_up(&gnetif))
	{
		  netif_set_up(&gnetif);
	}

	ethernetif_notify_conn_changed(&gnetif);
}

/* Function for sending RAW ethernet frame type 0x9999 */
void ETH_Send_data(const eth_address dst_mac, const uint8_t* data, uint16_t size)
{
	EthernetPayload_T payload;
	struct pbuf *p = NULL;

	payload.payload_length = size;
	payload.data = data;

	/* return the pointer to the prepared frame */
	p = Eth_CreateBuffer(&gnetif, source, dst_mac, &payload);

	if(p != NULL)
	{
		low_level_Tranmission(p);
	}

	/* release memory from the buffer */
	CLEAR_BUFFER(p)
}

/* check if new frame is available */
static void ETH_input_procces(void)
{
	  struct pbuf *p_buffer = NULL;

	  /* move received packet into a new pbuf */
	  p_buffer = get_data_buffer();

	  /* no packet could be read, silently ignore this */
	  if(p_buffer != NULL)
	  {
		  Ethernet_Read_Data(p_buffer);
		  free(p_buffer);
		  p_buffer = NULL;
	  }
}

/* Function for reading ethernet frames */
static void Ethernet_Read_Data(struct pbuf *p)
{
	if(p->payload != NULL)
	{
		EthernetPayload_T receive_data = {0};
		EthernetFrame_T raw_data = {0};

		memcpy(&raw_data, p->payload, ETHERNET_SIZE);

		int ret = memcmp(raw_data.EthernetLayer.dst_mac, source, MAC_ADDRESS_LENGTH);

		HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin, GPIO_PIN_RESET);


		if(ret == HAL_OK)
		{
			if(raw_data.EthernetLayer.type == IPV4_FRAME)
			{
				memcpy(&raw_data.InternetLayer, p->payload + ETHERNET_SIZE, IPV4_SIZE);

				if(raw_data.InternetLayer.protocol == UDP_FRAME)
				{
					memcpy(&raw_data.TransmisionLayer.udp_frame, p->payload  + TRANSMISSION_START, UDP_SIZE);
					receive_data.payload_length = RevertEndianness(raw_data.TransmisionLayer.udp_frame.length) - UDP_SIZE;
					if(receive_data.payload_length > 0)
					{
						receive_data.data = (uint8_t*)malloc(receive_data.payload_length * sizeof(uint8_t));
						memcpy(receive_data.data, p->payload + UDP_DATA_START, receive_data.payload_length);
					}
				}
				else if(raw_data.InternetLayer.protocol == TCP_FRAME)
				{
					memcpy(&raw_data.TransmisionLayer.tcp_frame, p->payload  + TRANSMISSION_START, TCP_SIZE);
					if(!CHECK_TCP_FLAG(raw_data, Syn) &&
						!CHECK_TCP_FLAG(raw_data, Fin) &&
						CHECK_TCP_FLAG(raw_data, Push))
					{
						receive_data.payload_length = RevertEndianness(raw_data.InternetLayer.totalLength) - TCP_SIZE - IPV4_SIZE;
						if(receive_data.payload_length > 0)
						{
							receive_data.data = (uint8_t*)malloc(receive_data.payload_length * sizeof(uint8_t));
							memcpy(receive_data.data, p->payload + TCP_DATA_START, receive_data.payload_length);
						}
					}
				}
			}
			else
			{
				/* RAW FRAMES */
				uint16_t length;
				memcpy(&length, p->payload + ETHERNET_SIZE, sizeof(uint16_t));
				receive_data.payload_length = RevertEndianness(length);
				if(receive_data.payload_length > 0)
				{
					receive_data.data = (uint8_t*)malloc(receive_data.payload_length);
					memcpy(receive_data.data, p->payload + ETHERNET_SIZE + sizeof(uint16_t), receive_data.payload_length);
					memcpy(receive_bytes, receive_data.data, receive_data.payload_length);
					HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin, GPIO_PIN_SET);
				}

			}
		}

		free(receive_data.data);
	}
}


/*****************************************************************************************************************
 * Do not modify the following part of the code
 ***************************************************************************************************************/
static void ethernetif_notify_conn_changed(struct netif *netif)
{
	if(netif_is_link_up(netif))
	{
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin,  GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin,  GPIO_PIN_RESET);
	}
	else
	{
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin,  GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin,  GPIO_PIN_SET);
	}
}

static uint16_t RevertEndianness(uint16_t data)
{
	uint16_t out = 0;

	for(uint8_t i = 0; i < sizeof(uint16_t); ++i)
	{
		out <<= 8;
		out |= data & 0xFF;
		data >>= 8;
	}

	return out;
}

static struct pbuf * Eth_CreateBuffer(struct netif* net, const eth_address src_mac, const eth_address dst_mac, const EthernetPayload_T* payload)
{
	struct pbuf *p = (struct pbuf *)malloc(sizeof(struct pbuf));
	EthernetFrame_T frame = {0};

	p->payload = (void*)malloc(ETHERNET_SIZE + payload->payload_length + sizeof(payload->payload_length));

	memcpy(frame.EthernetLayer.dst_mac, dst_mac, MAC_ADDRESS_LENGTH);
	memcpy(frame.EthernetLayer.src_mac, src_mac, MAC_ADDRESS_LENGTH);

	frame.EthernetLayer.type = RevertEndianness(RAW_FRAME);

	memcpy(p->payload, (void*)&frame.EthernetLayer, ETHERNET_SIZE);

	uint16_t length = RevertEndianness(payload->payload_length);

	memcpy(p->payload + ETHERNET_SIZE, &length, sizeof(uint16_t));

	memcpy(p->payload + ETHERNET_SIZE + sizeof(uint16_t), payload->data, payload->payload_length);

	heth.TxDesc->Buffer1Addr = (uint32_t)p->payload;
	heth.TxDesc->ControlBufferSize = ETHERNET_SIZE + sizeof(uint16_t) + payload->payload_length;
	p->len = p -> tot_len = ETHERNET_SIZE + payload->payload_length + sizeof(uint16_t);
	p->next = NULL;

	return p;
}


static struct pbuf * get_data_buffer(void)
{
  struct pbuf *p = NULL;
  struct pbuf *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dmarxdesc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i=0;

  /* get received frame */
  if (HAL_ETH_GetReceivedFrame(&heth) != HAL_OK)

    return NULL;

  /* Obtain the size of the packet and put it into the "len" variable. */
  len = heth.RxFrameInfos.length;
  buffer = (uint8_t *)heth.RxFrameInfos.buffer;

  if (len > 0)
  {
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL)
  {
    dmarxdesc = heth.RxFrameInfos.FSRxDesc;
    bufferoffset = 0;
    for(q = p; q != NULL; q = q->next)
    {
      byteslefttocopy = q->len;
      payloadoffset = 0;

      /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
      while( (byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE )
      {
        /* Copy data to pbuf */
        memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));

        /* Point to next descriptor */
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
        buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);

        byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }
      /* Copy remaining data in pbuf */
      memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
    }
  }

    /* Release descriptors to DMA */
    /* Point to first descriptor */
    dmarxdesc = heth.RxFrameInfos.FSRxDesc;
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (i=0; i< heth.RxFrameInfos.SegCount; i++)
    {
      dmarxdesc->Status |= ETH_DMARXDESC_OWN;
      dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }

    /* Clear Segment_Count */
    heth.RxFrameInfos.SegCount =0;

  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((heth.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)
  {
    /* Clear RBUS ETHERNET DMA flag */
    heth.Instance->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    heth.Instance->DMARPDR = 0;
  }
  return p;
}

static bool low_level_Tranmission(struct pbuf *p)
{
  struct pbuf *q;
  uint8_t *buffer = (uint8_t *)(heth.TxDesc->Buffer1Addr);
  __IO ETH_DMADescTypeDef *DmaTxDesc;
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;
  DmaTxDesc = heth.TxDesc;
  bufferoffset = 0;

  /* copy frame from pbufs to driver buffers */
  for(q = p; q != NULL; q = q->next)
    {
      /* Is this buffer available? If not, goto error */
      if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
      {
    	  ETH_ErrorHandle();
          return ERR_USE;
      }

      /* Get bytes in current lwIP buffer */
      byteslefttocopy = q->len;
      payloadoffset = 0;

      /* Check if the length of data to copy is bigger than Tx buffer size*/
      while( (byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE )
      {
        /* Copy data to Tx buffer*/
        memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset) );

        /* Point to next descriptor */
        DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

        /* Check if the buffer is available */
        if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
        {
        	ETH_ErrorHandle();
            return ERR_USE;
        }

        buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

        byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
        framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }

      /* Copy the remaining bytes */
      memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), byteslefttocopy );
      bufferoffset = bufferoffset + byteslefttocopy;
      framelength = framelength + byteslefttocopy;
    }

  /* Prepare transmit descriptors to give to DMA */
  HAL_ETH_TransmitFrame(&heth, framelength);

  return ERR_OK;
}


static void ETH_ErrorHandle(void)
{
  /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
  if ((heth.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET)
  {
	/* Clear TUS ETHERNET DMA flag */
	heth.Instance->DMASR = ETH_DMASR_TUS;

	/* Resume DMA transmission*/
	heth.Instance->DMATPDR = 0;
  }
}

#endif /* SRC_ETH_API_C_ */
