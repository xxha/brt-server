
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <time.h>
#include "main.h"
#include "pub.h"
#include "route.h"
#include "packet_monitor.h"

extern PROTO_OPS proto_ops[2];
extern SOCKET_INFO dev_socket[MAX_DEV_NAMES];
PROTO_TYPE_II vlan={0x81,0x00};
inline static void shift_vlan_buf(unsigned char *p,int *len){
	int pos,i;
	pos=ETHER_PROTO_OFF;

	//maybe need to add len compare.
	*len=0;
	for(i=0;i<3;i++){
		if((p[ETHER_PROTO_OFF+i*4]==vlan.hiByte)&&(p[ETHER_PROTO_OFF+i*4+1]==vlan.loByte))
			*len+=4;
	}
}

void *packet_monitor(void *para){


	SOCKET_INFO *sockPtr;
	struct ifreq ifr;
	struct timespec delay_req,delay_rem;
	
	struct sockaddr_ll peerSockAddr;
	int ret,i,len,pack_len,status;
	unsigned char *buf=NULL;
	unsigned char resultBuf[32];
	struct sockaddr_ll l2;
	int eth_dev_appear=0;
	unsigned char fileName[128],cmd[128];
	unsigned char host_ip[32];
	int vlan_len;
		
	if(para==NULL){//It should not read here ,only occur in the debug steps.
		printf("One can't make bricks without straw :).\n");
		exit(FAIL_ILLEGAL_PARA_FOR_THREAD);
	}
	sockPtr=(SOCKET_INFO *)para;
	sockPtr->status=STATUS_RUN;
	
	
	buf=malloc(MAX_FRAME_SIZE);
	if(buf==NULL){
		printf("Failed to alloc memory for socket data.\n");
		ret=ERR_ALLOC_MEM;
		sockPtr->status=STATUS_DEAD;
		pthread_exit(&ret);		
	}
	printf("monitor dev:%s sock:%d\n",sockPtr->name,sockPtr->sock);

	
	
	len=sizeof(peerSockAddr); 

BootUp:
	delay_req.tv_sec=0;
	delay_req.tv_nsec=200*1000*1000;//200ms
	while(1){//the device may disapper 
		
		nanosleep(&delay_req,&delay_rem);
		ret=bind_socket_to_devname(sockPtr);
		if(ret!=0){
			continue;
		}
		ret=get_dev_mask(sockPtr);
		if(ret!=0){
			continue;
		}		
		ret=get_dev_mac_address(sockPtr);
		if(ret!=0)
			continue;
		else
			break;
	}
	delay_req.tv_sec=0;
	delay_req.tv_nsec=100*1000*1000;//100ms
	while(1){

		//-- Check the exit command.
		if(sockPtr->cmd==CMD_EXIT){
			sockPtr->status=STATUS_DEAD;
			ret=-1;
			pthread_exit(&ret);
		}
		

		nanosleep(&delay_req,&delay_rem);
		
		memset(buf,0x00,sizeof(buf));
		ret=recvfrom(sockPtr->sock,buf,MAX_FRAME_SIZE,0,(struct sockaddr *)&peerSockAddr,&len);
		//ret=recvfrom(sockPtr->sock,buf,MAX_FRAME_SIZE,MSG_DONTWAIT,(struct sockaddr *)&peerSockAddr,&len);
	 	if(ret == -1){
	 		//perror(NULL);
	    	//printf("Error occur when reading data from %s socket\n",sockPtr->name);
	    	//ret=ERR_RCV_SOCK_DATA;
			//pthread_exit(&ret);
			goto BootUp;
		}
		pack_len=ret;

		if((0==pack_len)||(pack_len>MAX_FRAME_SIZE)){
			//printf("Length is illegal\n");
			continue;
		}
		
		/*filter the packet which is not for me.*/
		if(((buf[0]!=sockPtr->mac[0])||(buf[1]!=sockPtr->mac[1])||(buf[2]!=sockPtr->mac[2])||(buf[3]!=sockPtr->mac[3])||(buf[4]!=sockPtr->mac[4])||(buf[5]!=sockPtr->mac[5]))&&
		((buf[0]!=0xFF)||(buf[1]!=0xFF)||(buf[2]!=0xFF)||(buf[3]!=0xFF)||(buf[4]!=0xFF)|(buf[5]!=0xFF))){
			//printf("Discard:%2x %02x %2x %02x %2x %02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
			continue;//The packate is not for me ,discard it.
			}

		

#if 0
		
		int j,k;
		printf("\n");
		for(j=0,k=0;j<pack_len;j++,k++){
			if(k==16){
				printf("\n");
				k=0;
			}
			printf("%02x ",buf[j]);	
		}
		printf("\n");
		
#endif	
#ifdef DEBUG		
		printf("%s received %d bytes.\n",sockPtr->alias,pack_len);
#endif	
		
		
		//print_buf(buf,pack_len,16);
		//sleep(1);
		//printf("%s:%d\n",__FILE__,__LINE__);
		//printf("dev name:%s %08X\n",sockPtr->name,sockPtr->ip_addr);
		/* parse */
		status=-1;


#if 1
		vlan_len=0;
		shift_vlan_buf(buf,&vlan_len);
		
#endif			
		/*get my ip address*/
		strcpy(ifr.ifr_name,sockPtr->alias);
		ret=ioctl(sockPtr->sock,SIOCGIFADDR,&ifr);
		if(ret<0){
			//perror(NULL);
			//printf("%s:%d\n",__FILE__,__LINE__);
			//printf("Failed to retrive the interface ip address %s\n ",sockPtr->alias);
			continue;
		}
		
		sockPtr->ip_addr=(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
		for(i=0;i<sizeof(proto_ops);i++){					
			if((buf[ETHER_PROTO_OFF+vlan_len]==proto_ops[i].hiByte)&&(buf[ETHER_PROTO_OFF+vlan_len+1]==proto_ops[i].loByte)){
				//memset(resultBuf,0x00,sizeof(resultBuf));
				//printf("%s:%d %d\n",__FILE__,__LINE__,i);
				ret=proto_ops[i].parse(buf,pack_len,vlan_len,sockPtr->ip_addr,sockPtr->alias,resultBuf);

				if(ret==0)
					status=0;

				break;
			}			
		}
		if(status==-1)/* not hit */{
#ifdef DEBUG	
		printf("%s:%d\n",__FILE__,__LINE__);		
#endif		
			continue;
		}
#ifdef DEBUG			
		//printf("%s\n",resultBuf);
		printf("%s:%d %s\n",__FILE__,__LINE__,resultBuf);
#endif	
		
		//printf("%s\n",resultBuf);
		//printf("Threadid:%d,%s %d.\n",pthread_self(),sockPtr->name,sockPtr->ip_addr);
		/*add route */	
		if(!inet_ntop(AF_INET,&sockPtr->ip_addr,host_ip,sizeof(host_ip))){
			perror(NULL);
			return;
		}
		keep_route(resultBuf,sockPtr->netmask,sockPtr->gateway,sockPtr->alias,host_ip);
		//printf("Threadid:%d,%s %d.\n",pthread_self(),sockPtr->name,sockPtr->ip_addr);
	}
}
int get_dev_mac_address(SOCKET_INFO *sockInfo){
	struct ifreq ifr; 
	int ret;


	//printf("%s:%d-%s\n",__FILE__,__LINE__,sockInfo->name);
	strcpy(ifr.ifr_name,sockInfo->name);
	ret=ioctl(sockInfo->sock,SIOCGIFHWADDR,&ifr);
	if(ret<0){
		//perror(NULL);
		//printf("Failed to get mac address.\n");
		//printf("%s:%d-%s\n",__FILE__,__LINE__,ifr.ifr_name);
		return -1;
	}
#if 0	
	printf("device:%s Mac:%02x:%02x:%02x:%02x:%02x:%02x\n",sockInfo->alias,
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[0],
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[1],
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[2],
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[3],
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[4],
	  	(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
#endif	  	
	sockInfo->mac[0]=(unsigned char)ifr.ifr_hwaddr.sa_data[0];
	sockInfo->mac[1]=(unsigned char)ifr.ifr_hwaddr.sa_data[1];
	sockInfo->mac[2]=(unsigned char)ifr.ifr_hwaddr.sa_data[2];			
	sockInfo->mac[3]=(unsigned char)ifr.ifr_hwaddr.sa_data[3];			
	sockInfo->mac[4]=(unsigned char)ifr.ifr_hwaddr.sa_data[4];			
	sockInfo->mac[5]=(unsigned char)ifr.ifr_hwaddr.sa_data[5];		

	return 0;

}
int get_dev_mask(SOCKET_INFO *sockInfo){
	struct ifreq ifr; 
	int ret;


	//printf("%s:%d-%s\n",__FILE__,__LINE__,sockInfo->name);
	strcpy(ifr.ifr_name,sockInfo->name);
	ret=ioctl(sockInfo->sock,SIOCGIFNETMASK,&ifr);
	if(ret<0){
		//perror(NULL);
		//printf("Failed to get net mask address.\n");
		//printf("%s:%d-%s\n",__FILE__,__LINE__,ifr.ifr_name);
		return -1;
	}
	snprintf(sockInfo->netmask,sizeof(sockInfo->netmask),"%d.%d.%d.%d",(unsigned char)ifr.ifr_hwaddr.sa_data[2],
		(unsigned char)ifr.ifr_hwaddr.sa_data[3],
		(unsigned char)ifr.ifr_hwaddr.sa_data[4],
		(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
#if 0	
	printf("net mask:%s\n",sockInfo->netmask);	
#endif	  

	return 0;

}

/*
0:success.
-1:failed to get net device index.*/
int bind_socket_to_devname(SOCKET_INFO *sockInfo){
	unsigned char cmd[256];
	struct ifreq ifr; 
	int ret;

	struct sockaddr_ll l2;
	/*bind socket to device*/
	strcpy(ifr.ifr_name,sockInfo->alias);		
	ret=ioctl(sockInfo->sock,SIOCGIFINDEX,&ifr);
	if(ret<0){
		//printf("%s:%d Failed to retrive the interface index of %s\n ",__FILE__,__LINE__,ifr.ifr_name);
		return -1;
	}
	l2.sll_family=AF_PACKET;
	l2.sll_protocol=htons(ETH_P_ALL);
	l2.sll_ifindex=ifr.ifr_ifindex;		
	//printf("%d\n",ifr[i].ifr_ifindex);
	ret=bind(sockInfo->sock,(struct sockaddr *)&l2,sizeof(l2));
	if(ret<0){
		//printf("Failed to bind %s to socket %d.\n",sockInfo->alias,sockInfo->sock);
		return -2;
	}
	return 0;	
}
