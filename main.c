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

#include <signal.h>

#include "pub.h"
#include "route.h"
#include "ip.h"
#include "arp.h"

#include "scan_dev_name.h"
#include "main.h"
#include "packet_monitor.h"
#include "ipc_monitor.h"

SOCKET_INFO dev_socket[MAX_DEV_NAMES]={

	{
		.sock=-1,
		.name="eth0",
		.alias="eth0",
		.gateway="0.0.0.0",
 		.index=0,
 		.update=1,
 		.cmd=CMD_RUN,
 		.status=STATUS_IDEL,
 		.work=0,
 		.order=0,
	},

	{
		.sock=-1,
		.name="eth1",
		.alias="eth1",
		.gateway="0.0.0.0",
 		.index=0,
 		.update=1,
 		.cmd=CMD_RUN,
 		.status=STATUS_IDEL,
 		.work=0,
 		.order=1,
	},

	{
		.sock=-1,
		.name="eth3",
		.alias="eth3",
		.gateway="0.0.0.0",
 		.index=0,
 		.update=1, 	
 		.cmd=CMD_RUN,
 		.status=STATUS_IDEL,
 		.work=0,
 		.order=2,
	}

};

#ifndef DROUTE

PROTO_OPS proto_ops[2]={

#ifdef ETH_IP
	{0x08,0x00,parse_ip},
#endif

#ifdef ETH_ARP
	{0x08,0x06,parse_arp},
#endif
};

#endif

IPC_MONITOR_INFO ipcMonitorInfo={
	.cmd=CMD_RUN,
 	.status=STATUS_IDEL,

};

extern unsigned int g_netadd[3];

inline static void shift_vlan_buf(unsigned char *p,int *len);
void *droute_process(void *para);

int DEV_COUNTS=sizeof(dev_socket)/sizeof(SOCKET_INFO);
int main(int argc,char *argv[]){


	int i,j,k,ret;
	struct ifreq ifr[DEV_COUNTS];
	struct sockaddr_ll l2[DEV_COUNTS];
	pthread_t thread_sock[DEV_COUNTS],thread_ipc;
	pthread_attr_t attr[DEV_COUNTS],attr_ipc;
	void *retValue;
	struct sockaddr_in *my_sock_addr;
	struct timespec delay_req,delay_rem;
	int thread_counts;
	
	pthread_attr_t scan_net_dev_name_attr;
	pthread_t scan_net_dev_name_thread;
	
#ifdef VERSION
        printf("\nVersion:%s ", VERSION);
#endif
#ifdef AUTHOR
        printf("Author:%s\n",AUTHOR);
#endif
	printf("%s ,compiled at %s %s.\n",argv[0],__DATE__,__TIME__);

	/* deal with zobie process */
	struct sigaction sa; 
	sa.sa_handler = SIG_IGN; 
	sa.sa_flags = SA_NOCLDWAIT; 
	sigemptyset(&sa.sa_mask); 
	sigaction(SIGCHLD, &sa, NULL); 
	
	/* open the route table file */
	ret = open_route_file();
	if(0 != ret) {
		printf("Failed to open the route table info file.\n");
		exit(FAIL_TO_OPEN_ROUTE_FILE);
	}


	/* create the ipc thread */
	ret = pthread_attr_init(&attr_ipc);
	if(ret != 0) {
		printf("Faile to init ipc thread attribute. %d .\n ");
		ret = ERR_INIT_THREAD_ATTR;
		goto Out2;
	}
	ret = pthread_create(&thread_ipc,&attr_ipc,ipc_monitor,&ipcMonitorInfo);
	if(ret !=0 ) {
			printf("Faile to create ipc thread.\n ");
			ret = ERR_CREATE_THREAD_IPC;
			goto Out2;
	}
	
	/* create socket */
	for(i=0;i<MAX_DEV_NAMES;i++){
	#ifdef DROUTE
		printf("droute process will run on the %d interface.\n",i);
	#else
		dev_socket[i].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL ));
		if(dev_socket[i].sock<0){
			printf("Failed to create socket for %s .\n",dev_socket[i].name);
			ret = ERR_CREATE_SOCKET;
			goto Out2;
		}	
	#endif

#if 0		
		ret = get_dev_mac_address(&dev_socket[i]);
		if(ret != 0) {
			printf("%s:%d-%s\n",__FILE__,__LINE__,"Failed to device mac.");
			return -1;
		}
		/* bind the socket to the device. */
		ret = bind_socket_to_devname(&dev_socket[i]);
		if(ret != 0) {
			printf("%s:%d-%s\n",__FILE__,__LINE__,"Failed to bind socket to device.");
		}
#endif	

		ret=pthread_attr_init(&attr[i]);
		if(ret!=0){
			printf("%s:%d-Faile to init thread attribute. %d .\n ",__FILE__,__LINE__);
			return -3;
		}
#ifdef DROUTE
		g_netadd[i] = 0;
		printf("use pcap mechanism,so don't need to use RAW to process the packets\n");
		ret=pthread_create(&thread_sock[i],&attr[i],droute_process,&dev_socket[i]);
		if(ret!=0){
			printf("%s:%d-Faile to init thread attribute. %d .\n ",__FILE__,__LINE__);
			return -4;
		}
#else
		ret=pthread_create(&thread_sock[i],&attr[i],packet_monitor,&dev_socket[i]);
		if(ret!=0){
			printf("%s:%d-Faile to init thread attribute. %d .\n ",__FILE__,__LINE__);
			return -4;
		}
#endif
	}
	
	/* wait to exit */
	delay_req.tv_sec=0;
	delay_req.tv_nsec=500*1000*1000;
	thread_counts=DEV_COUNTS+1;/* DEV_COUNTS  socket thread and one ipc monitor thread. */
	while(1){		
		nanosleep(&delay_req,&delay_rem);
		for(i=0;i<DEV_COUNTS;i++){
			if(dev_socket[i].status==STATUS_DEAD)
				break;
		}
		if(i<DEV_COUNTS)
			break;/* jump out of the while cycle */
		if(ipcMonitorInfo.status==STATUS_DEAD)
			break;/* jump out of the while cycle */
	}

	for(i=0;i<DEV_COUNTS;i++){
		dev_socket[i].cmd=CMD_EXIT;
	}
	ipcMonitorInfo.cmd=CMD_EXIT;
	while(1){
		nanosleep(&delay_req,&delay_rem);
		i=0;
		for(j=0;j<DEV_COUNTS;j++){
			if(dev_socket[j].status==STATUS_DEAD)
				i++;
		}
		if(ipcMonitorInfo.status==STATUS_DEAD)
			i++;
		if(i<thread_counts)
			continue;
		else
			break;
	}
Out2:	
	close_route_file();		
	for(i=0;i<DEV_COUNTS;i++){
		if(dev_socket[i].sock!=-1){
			close(dev_socket[i].sock);
			dev_socket[i].sock=-1;
		}		
	}
	return ret;
}

