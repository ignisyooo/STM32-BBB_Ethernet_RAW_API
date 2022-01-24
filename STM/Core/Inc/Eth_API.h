/*
 * Eth_API.h
 *
 *  Created on: 23 lis 2021
 *      Author: rzesz
 */

#ifndef INC_ETH_API_H_
#define INC_ETH_API_H_

#include <stdbool.h>

#include "netif.h"
#include "lwip/pbuf.h"

#define MAX_DATA_SIZE					1000U

#define MAC_ADDRESS_LENGTH 				6U

#define ETHERNET_SIZE 					14U
#define IPV4_SIZE						20U
#define UDP_SIZE						8U
#define TCP_SIZE						20U

#define UDP_FRAME						0x11U
#define TCP_FRAME						0x06U
#define IPV4_FRAME						(uint16_t)0x0800
#define RAW_FRAME						(uint16_t)0x9999


#define TRANSMISSION_START				(ETHERNET_SIZE + IPV4_SIZE)
#define UDP_DATA_START					(ETHERNET_SIZE + IPV4_SIZE + UDP_SIZE)
#define TCP_DATA_START					(ETHERNET_SIZE + IPV4_SIZE + TCP_SIZE)

#define CHECK_TCP_FLAG(data, flag)		(data.TransmisionLayer.tcp_frame.flags.flag)

#define GET_ARRAY_SIZE(array)			(sizeof(array) / sizeof(array[0]))

#define CLEAR_BUFFER(buffer)	 		 free(buffer->payload); \
										 free(buffer); \
										 buffer = NULL;

typedef uint8_t eth_address[MAC_ADDRESS_LENGTH];

typedef struct
{
	uint8_t CWR : 1;
	uint8_t ECHO : 1;
	uint8_t Urgent : 1;
	uint8_t ACK : 1;
	uint8_t Push : 1;
	uint8_t Reset : 1;
	uint8_t Syn : 1;
	uint8_t Fin : 1;
}TCP_Flags;

typedef struct
{
	uint16_t sourcePort;
	uint16_t destPort;
	uint32_t sequenceNumber;
	uint32_t ack_number;
	uint8_t header_length;
	TCP_Flags flags;
	uint16_t window;
	uint16_t crc;
	uint16_t urgent;
}TCP;

typedef struct
{
	uint16_t sourcePort;
	uint16_t destPort;
	uint16_t length;
	uint16_t checksum;
}UDP;

typedef union
{
	UDP udp_frame;
	TCP tcp_frame;
}TransmissionControlProtocol;

typedef struct
{
	uint8_t headerLength : 4;
	uint8_t version : 4;
	uint8_t DiffrentServiceField;
	uint16_t totalLength;
	uint16_t identifcation;
	uint8_t flags;
	uint8_t offset;
	uint8_t timetolive;
	uint8_t protocol;
	uint16_t checksum;
	uint8_t ip_src[4];
	uint8_t ip_dest[4];
}InternetProtocol;

typedef struct
{
	eth_address dst_mac;
	eth_address src_mac;
	uint16_t type;
}Ethernet_T;

#pragma pack(push, 1)
typedef struct
{
	Ethernet_T EthernetLayer;
	InternetProtocol InternetLayer;
	TransmissionControlProtocol TransmisionLayer;
}EthernetFrame_T;
#pragma pack(pop)


typedef struct
{
	uint8_t* data;
	size_t payload_length;
} EthernetPayload_T;


void ETH_Proccess(void);
void ETH_Send_data(const eth_address dst_mac, const uint8_t* data, uint16_t size);


#endif /* INC_ETH_API_H_ */
