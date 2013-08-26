#include <stdio.h>
#include "pub.h"
#define SEND_IP_OFF 28
#define RCV_IP_OFF 	38
int parse_arp(unsigned char *buf,int len,int vlan_len,unsigned int ip_addr,unsigned char *dev_name,unsigned char *result){
	unsigned char myIP[4];
	int i;

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


	for(i=0;i<4;i++){
		if(buf[RCV_IP_OFF+vlan_len+i]!=myIP[i]){
#ifdef DEBUG		
			printf("%d :my:%d .rcv:%d\n",i,myIP[i],buf[RCV_IP_OFF+i]);
#endif			
			return 1;
		}
	}	
	sprintf(result,"%d.%d.%d.%d",buf[SEND_IP_OFF+vlan_len],buf[SEND_IP_OFF+vlan_len+1],buf[SEND_IP_OFF+vlan_len+2],buf[SEND_IP_OFF+vlan_len+3]);
	return 0;
}
