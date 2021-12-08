/*
 * Eth_API.c
 *
 *  Created on: 23 lis 2021
 *      Author: rzeszutko
 */

#ifndef SRC_ETH_API_C_
#define SRC_ETH_API_C_


#include "ethernet.h"
#include "ethernetif.h"
#include "Eth_API.h"
#include "timeouts.h"
#include <string.h>


extern ETH_HandleTypeDef heth;
extern struct netif gnetif;

static const eth_address source = { 0x00, 0x80, 0xE2, 0x00, 0x00, 0x00};
//static const eth_address destination = {0x00, 0x80, 0xE1, 0x00, 0x00, 0x00};

static uint16_t RevertEndianness(uint16_t data);
static void ETH_input_procces(struct netif *netif);
static void ethernetif_notify_conn_changed(struct netif *netif);
static void Ethernet_Read_Data(struct pbuf *p);
static struct pbuf * Ethernet_Send_Data(struct netif* net, const eth_address src_mac, const eth_address dst_mac, const EthernetPayload_T* payload);

void ETH_Proccess(void)
{
	/* check input data */
	ETH_input_procces(&gnetif);

	/* Handle timeouts */
	sys_check_timeouts();

	ethernetif_set_link(&gnetif);

	if(netif_is_link_up(&gnetif) && !netif_is_up(&gnetif))
	{
		  netif_set_up(&gnetif);
	}

	ethernetif_notify_conn_changed(&gnetif);
}

void ETH_Send_data(const eth_address dst_mac, uint8_t* data, uint16_t size)
{
	EthernetPayload_T payload;
	struct pbuf *p;
	payload.payload_length = size;


	payload.data = data;

	p = Ethernet_Send_Data(&gnetif, source, dst_mac, &payload);

	if(p != NULL)
	{
		low_level_output(&gnetif, p);
	}

	free(p->payload);
	free(p);
	p = NULL;
	payload.data = NULL;
}


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

static struct pbuf * Ethernet_Send_Data(struct netif* net, const eth_address src_mac, const eth_address dst_mac, const EthernetPayload_T* payload)
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

static void ETH_input_procces(struct netif *netif)
{
	  struct pbuf *p;

	  /* move received packet into a new pbuf */
	  p = low_level_input(netif);

	  /* no packet could be read, silently ignore this */
	  if(p != NULL)
	  {
		  Ethernet_Read_Data(p);
		  free(p);
		  p = NULL;
	  }
}


static void Ethernet_Read_Data(struct pbuf *p)
{
	if(p->payload != NULL)
	{
		EthernetPayload_T receive_data = {0};
		EthernetFrame_T raw_data = {0};

		int ret = 0;


		memcpy(&raw_data, p->payload, ETHERNET_SIZE);

		ret = memcmp(raw_data.EthernetLayer.dst_mac, source, MAC_ADDRESS_LENGTH);

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
					HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin, GPIO_PIN_SET);
				}

			}
		}

		free(receive_data.data);
	}
}



#endif /* SRC_ETH_API_C_ */
