#include <stdio.h>
#include "pub.h"
#define SEND_IP_OFF 26
#define RCV_IP_OFF 	30
#define UDP_PORT_OFF 0x24
#define ETH_DST_MAC 0x00
unsigned char softupdate_udp_port[]={0x2b,0x5c};
unsigned char discovery_udp_port[]={0x80,0x18};
int parse_ip(unsigned char *buf,int len,int vlan_len,unsigned int ip_addr,unsigned char *dev_name,unsigned char *result){
	unsigned char myIP[4];
	int i,ret;
	//printf("%s:%d %02X %02X\n",__FILE__,__LINE__,buf[RCV_IP_OFF+3],buf[SEND_IP_OFF+3]);
	for(i=0;i<4;i++){
		myIP[i]=ip_addr&0xFF;
		ip_addr=ip_addr>>8;
#ifdef DEBUG		
		printf("%2X ",myIP[i]);
#endif		
	}
#ifdef DEBUG	
	printf("\n");
#endif
	if((buf[RCV_IP_OFF+vlan_len]==0xFF)&&(buf[RCV_IP_OFF+vlan_len+1]==0xFF)&&(buf[RCV_IP_OFF+vlan_len+2]==0xFF)&&(buf[RCV_IP_OFF+vlan_len+3]==0xFF)){
		//printf("%s:%d %02x %02x %02x %02x\n",__FILE__,__LINE__,buf[RCV_IP_OFF+vlan_len+0],buf[RCV_IP_OFF+vlan_len+1],buf[RCV_IP_OFF+vlan_len+2],buf[RCV_IP_OFF+vlan_len+3]);
		//if ,it is the broadcast for softupdate. only bind it to manage port.
		if((buf[UDP_PORT_OFF+vlan_len]==softupdate_udp_port[0])&&(buf[UDP_PORT_OFF+vlan_len+1]==softupdate_udp_port[1])&&(strcmp(dev_name,MANAGE_PORT)!=0))			
				return 1;//discard it.
		else
			goto Match;//softupdate package.
	}

	if((buf[ETH_DST_MAC+vlan_len]==0xFF)&&(buf[ETH_DST_MAC+vlan_len+1]==0xFF)&&(buf[ETH_DST_MAC+vlan_len+2]==0xFF)&&
		(buf[ETH_DST_MAC+vlan_len+3]==0xFF)&&(buf[ETH_DST_MAC+vlan_len+4]==0xFF)&&(buf[ETH_DST_MAC+vlan_len+5]==0xFF)){	
		//printf("%s:%d %02x %02x %02x %02x\n",__FILE__,__LINE__,buf[RCV_IP_OFF+vlan_len+0],buf[RCV_IP_OFF+vlan_len+1],buf[RCV_IP_OFF+vlan_len+2],buf[RCV_IP_OFF+vlan_len+3]);	
		//if , it is the broadcast for discovery, only bind it to the test port.
		if((buf[UDP_PORT_OFF+vlan_len]==discovery_udp_port[0])&&(buf[UDP_PORT_OFF+vlan_len+1]==discovery_udp_port[1])&&(strcmp(dev_name,MANAGE_PORT)==0))			
				return 1;//discard it. don 't bind the discovery  on the management port.
		else
			goto Match;//broradcast package. for discovery.
		//printf("%s:%d %02x %02x %02x %02x\n",__FILE__,__LINE__,buf[RCV_IP_OFF+vlan_len+0],buf[RCV_IP_OFF+vlan_len+1],buf[RCV_IP_OFF+vlan_len+2],buf[RCV_IP_OFF+vlan_len+3]);
	}
		
	for(i=0;i<4;i++){
		if(buf[RCV_IP_OFF+vlan_len+i]!=myIP[i]){
#ifdef DEBUG		
			printf("%d :my:%d .rcv:%d\n",i,myIP[i],buf[RCV_IP_OFF+vlan_len+i]);
#endif			
			return 1;
		}
	}
Match:	
	sprintf(result,"%d.%d.%d.%d",buf[SEND_IP_OFF+vlan_len],buf[SEND_IP_OFF+vlan_len+1],buf[SEND_IP_OFF+vlan_len+2],buf[SEND_IP_OFF+vlan_len+3]);
	return 0;
}
