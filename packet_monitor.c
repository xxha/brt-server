
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
PROTO_TYPE_II vlan = {0x81,0x00};

inline static void shift_vlan_buf(unsigned char *p,int *len)
{
	int pos,i;
	pos = ETHER_PROTO_OFF;

	//maybe need to add len compare.
	*len = 0;
	for(i=0; i<3; i++){
		if((p[ETHER_PROTO_OFF + i * 4] == vlan.hiByte) && (p[ETHER_PROTO_OFF + i * 4 + 1] == vlan.loByte))
			*len += 4;
	}
}

void *packet_monitor(void *para)
{
	int vlan_len;
	int eth_dev_appear = 0;
	int i, ret, len, pack_len, status;
	unsigned char *buf = NULL;
	unsigned char resultBuf[32];
	unsigned char fileName[128], cmd[128];
	unsigned char host_ip[32];
	struct ifreq ifr;
	struct timespec delay_req, delay_rem;	
	struct sockaddr_ll peerSockAddr;
	struct sockaddr_ll l2;
	SOCKET_INFO *sockPtr;
	
	if(para == NULL) {//It should not read here ,only occur in the debug steps.
		printf("One can't make bricks without straw :).\n");
		exit(FAIL_ILLEGAL_PARA_FOR_THREAD);
	}
	sockPtr = (SOCKET_INFO *)para;
	sockPtr->status = STATUS_RUN;
	
	
	buf = malloc(MAX_FRAME_SIZE);
	if(buf == NULL) {
		printf("Failed to alloc memory for socket data.\n");
		ret = ERR_ALLOC_MEM;
		sockPtr->status = STATUS_DEAD;
		pthread_exit(&ret);		
	}
	printf("monitor dev:%s sock:%d\n", sockPtr->name, sockPtr->sock);

	
	
	len = sizeof(peerSockAddr);

BootUp:
	delay_req.tv_sec = 0;
	delay_req.tv_nsec = 100*1000*1000;//100ms
	while(1) {
		if(!sockPtr->work) {
			nanosleep(&delay_req, &delay_rem);
			continue;
		}
		bind_socket_to_devname(sockPtr);
		
		if(sockPtr->cmd == CMD_EXIT){
			sockPtr->status = STATUS_DEAD;
			ret = -1;
			pthread_exit(&ret);
		}
		
		nanosleep(&delay_req, &delay_rem);
		
		memset(buf, 0x00, sizeof(buf));
		ret = recvfrom(sockPtr->sock, buf, MAX_FRAME_SIZE, 0, (struct sockaddr *)&peerSockAddr, &len);
	 	if(ret == -1) {
	 		perror(NULL);
	    		printf("Error occur when reading data from %s socket\n",sockPtr->name);
			goto BootUp;
		}
		pack_len = ret;

		if((0 == pack_len) || (pack_len > MAX_FRAME_SIZE)) {
			continue;
		}

		/*filter the packet which is not for me.*/
		if(((buf[0] != sockPtr->mac[0]) || (buf[1] != sockPtr->mac[1])
			|| (buf[2] != sockPtr->mac[2]) || (buf[3] != sockPtr->mac[3])
			|| (buf[4] != sockPtr->mac[4]) || (buf[5] != sockPtr->mac[5]))
			&& ((buf[0]!=0xFF)||(buf[1]!=0xFF)||(buf[2]!=0xFF)
			||(buf[3]!=0xFF)||(buf[4]!=0xFF)|(buf[5]!=0xFF))) {
			continue;//The packate is not for me ,discard it.
		}

		int j,k;
		DEBUG("\n");
		for(j=0, k=0; j<pack_len; j++, k++) {
			if(k == 16) {
				DEBUG("\n");
				k = 0;
			}
			DEBUG("%02x ", buf[j]);	
		}
		DEBUG("\n");
		DEBUG("%s received %d bytes.\n", sockPtr->alias, pack_len);
		
		/* parse */
		status = -1;
		vlan_len = 0;
		shift_vlan_buf(buf,&vlan_len);

		inet_pton(AF_INET, sockPtr->ip_deci_dot, &sockPtr->ip_addr);
		for(i=0; i<sizeof(proto_ops); i++){					
			if((buf[ETHER_PROTO_OFF+vlan_len] == proto_ops[i].hiByte)
				&& (buf[ETHER_PROTO_OFF+vlan_len+1] == proto_ops[i].loByte)){
				ret = proto_ops[i].parse(buf, pack_len, vlan_len, sockPtr->ip_addr, sockPtr->alias, resultBuf);
				if(ret==0)
					status=0;
				break;
			}	
		}
		if(status==-1) {
			DEBUG("%s:%d\n",__FILE__,__LINE__);		
			continue;
		}
		DEBUG("%s\n", resultBuf);
		
		/*add route */	
		keep_route(resultBuf, sockPtr->netmask, sockPtr->gateway, sockPtr->alias, sockPtr->ip_deci_dot);
	}
}

int get_dev_mac_address(SOCKET_INFO *sockInfo)
{
	int ret;
	struct ifreq ifr; 

	strcpy(ifr.ifr_name, sockInfo->name);
	ret = ioctl(sockInfo->sock, SIOCGIFHWADDR, &ifr);
	if(ret < 0) {
		return -1;
	}

	DEBUG("device:%s Mac:%02x:%02x:%02x:%02x:%02x:%02x\n",sockInfo->alias,
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[0],
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[1],
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[2],
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[3],
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[4],
				  	(unsigned char)ifr.ifr_hwaddr.sa_data[5]);

	sockInfo->mac[0] = (unsigned char)ifr.ifr_hwaddr.sa_data[0];
	sockInfo->mac[1] = (unsigned char)ifr.ifr_hwaddr.sa_data[1];
	sockInfo->mac[2] = (unsigned char)ifr.ifr_hwaddr.sa_data[2];			
	sockInfo->mac[3] = (unsigned char)ifr.ifr_hwaddr.sa_data[3];			
	sockInfo->mac[4] = (unsigned char)ifr.ifr_hwaddr.sa_data[4];			
	sockInfo->mac[5] = (unsigned char)ifr.ifr_hwaddr.sa_data[5];		

	return 0;

}

int get_dev_mask(SOCKET_INFO *sockInfo)
{
	int ret;
	struct ifreq ifr; 

	strcpy(ifr.ifr_name, sockInfo->name);
	ret = ioctl(sockInfo->sock, SIOCGIFNETMASK, &ifr);
	if(ret < 0) {
		return -1;
	}

	snprintf(sockInfo->netmask, sizeof(sockInfo->netmask), "%d.%d.%d.%d", (unsigned char)ifr.ifr_hwaddr.sa_data[2],
		(unsigned char)ifr.ifr_hwaddr.sa_data[3],
		(unsigned char)ifr.ifr_hwaddr.sa_data[4],
		(unsigned char)ifr.ifr_hwaddr.sa_data[5]);

	DEBUG("net mask:%s\n",sockInfo->netmask);	

	return 0;

}

int bind_socket_to_devname(SOCKET_INFO *sockInfo)
{
	int ret;
	unsigned char cmd[256];
	struct ifreq ifr; 
	struct sockaddr_ll l2;

	/*bind socket to device*/
	strcpy(ifr.ifr_name, sockInfo->alias);

	ret = ioctl(sockInfo->sock, SIOCGIFINDEX, &ifr);
	if(ret < 0) {
		return -1;
	}

	l2.sll_family = AF_PACKET;
	l2.sll_protocol = htons(ETH_P_ALL);
	l2.sll_ifindex = ifr.ifr_ifindex;		

	ret = bind(sockInfo->sock, (struct sockaddr *)&l2, sizeof(l2));
	if(ret < 0) {
		return -2;
	}

	return 0;	
}
