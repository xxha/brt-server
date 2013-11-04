#include <stdio.h>
#include "main.h"

#define SEND_IP_OFF	28
#define RCV_IP_OFF	38
#define UDP_PORT_OFF    0x24
#define ETH_DST_MAC     0x00

unsigned char softupdate_udp_port[] = {0x2b,0x5c};
unsigned char discovery_udp_port[] = {0x80,0x18};

int parse_arp(unsigned char *buf,int len,int vlan_len,unsigned int ip_addr,unsigned char *dev_name,unsigned char *result)
{
	int i;
	unsigned char myIP[4];

	for(i=0;i<4;i++){
		myIP[i]=ip_addr&0xFF;
		ip_addr=ip_addr>>8;
		DEBUG("%2X ",myIP[i]);
	}
	DEBUG("\n");

	for(i=0;i<4;i++){
		if(buf[RCV_IP_OFF+vlan_len+i]!=myIP[i]){
			DEBUG("%d :my:%d .rcv:%d\n",i,myIP[i],buf[RCV_IP_OFF+i]);
			return 1;
		}
	}	

	sprintf(result,"%d.%d.%d.%d",buf[SEND_IP_OFF+vlan_len],buf[SEND_IP_OFF+vlan_len+1],buf[SEND_IP_OFF+vlan_len+2],buf[SEND_IP_OFF+vlan_len+3]);

	return 0;
}

int parse_ip(unsigned char *buf, int len, int vlan_len, unsigned int ip_addr,
				unsigned char *dev_name, unsigned char *result)
{
	int i,ret;
	unsigned char myIP[4];

	for(i=0; i<4; i++) {
		myIP[i] = ip_addr&0xFF;
		ip_addr = ip_addr>>8;
		DEBUG("%2X ",myIP[i]);
	}
	DEBUG("\n");

	if((buf[RCV_IP_OFF+vlan_len] == 0xFF) && (buf[RCV_IP_OFF+vlan_len+1] == 0xFF)
		&& (buf[RCV_IP_OFF+vlan_len+2] == 0xFF) && (buf[RCV_IP_OFF+vlan_len+3] == 0xFF)) {
		/* if ,it is the broadcast for softupdate. only bind it to manage port. */
		if((buf[UDP_PORT_OFF+vlan_len] == softupdate_udp_port[0])
			&& (buf[UDP_PORT_OFF+vlan_len+1] == softupdate_udp_port[1])
			&& (strcmp(dev_name,MANAGE_PORT) != 0))
				return 1;//discard it.
		else
			goto Match;//softupdate package.
	}

	if((buf[ETH_DST_MAC+vlan_len] == 0xFF) && (buf[ETH_DST_MAC+vlan_len+1] == 0xFF)
		&& (buf[ETH_DST_MAC+vlan_len+2] == 0xFF) && (buf[ETH_DST_MAC+vlan_len+3] == 0xFF)
		&& (buf[ETH_DST_MAC+vlan_len+4] == 0xFF) && (buf[ETH_DST_MAC+vlan_len+5] == 0xFF)) {
		/* if , it is the broadcast for discovery, only bind it to the test port. */
		if((buf[UDP_PORT_OFF+vlan_len] == discovery_udp_port[0])
			&& (buf[UDP_PORT_OFF+vlan_len+1] == discovery_udp_port[1])
			&& (strcmp(dev_name,MANAGE_PORT) == 0))
				return 1;//discard it. don 't bind the discovery  on the management port.
		else
			goto Match;//broradcast package. for discovery.
	}

	for(i=0;i<4;i++){
		if(buf[RCV_IP_OFF+vlan_len+i]!=myIP[i]){
			DEBUG("%d :my:%d .rcv:%d\n", i, myIP[i], buf[RCV_IP_OFF+vlan_len+i]);
			return 1;
		}
	}
Match:
	sprintf(result, "%d.%d.%d.%d", buf[SEND_IP_OFF+vlan_len], buf[SEND_IP_OFF+vlan_len+1],
					buf[SEND_IP_OFF+vlan_len+2], buf[SEND_IP_OFF+vlan_len+3]);
	return 0;
}

