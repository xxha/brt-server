/*
 * brt-server --- abbreviation of Bind Route Server
 * which dynamicly configure route to different ethernet interface(ethx, x = 0-2)
*/

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

#include "route.h"
#include "parse.h"
#include "scan_dev_name.h"
#include "main.h"
#include "packet_monitor.h"
#include "ipc_monitor.h"

SOCKET_INFO dev_socket[MAX_DEV_NAMES] = {
	{
		.sock = -1,
		.name = "eth0",
		.alias = "eth0",
		.gateway = "0.0.0.0",
 		.index = 0,
 		.update = 1,
 		.cmd = CMD_RUN,
 		.status = STATUS_IDEL,
 		.work = 0,
 		.order = 0,
	},
	{
		.sock = -1,
		.name = "eth1",
		.alias = "eth1",
		.gateway = "0.0.0.0",
 		.index = 0,
 		.update = 1,
 		.cmd = CMD_RUN,
 		.status = STATUS_IDEL,
 		.work = 0,
 		.order = 1,
	},
	{
		.sock = -1,
		.name = "eth3",
		.alias = "eth3",
		.gateway = "0.0.0.0",
 		.index = 0,
 		.update = 1, 	
 		.cmd = CMD_RUN,
 		.status = STATUS_IDEL,
 		.work = 0,
 		.order = 2,
	}
};

#ifndef DARP
PROTO_OPS proto_ops[2] = {
#ifdef ETH_IP
	{0x08,0x00,parse_ip},
#endif
#ifdef ETH_ARP
	{0x08,0x06,parse_arp},
#endif
};
#endif

IPC_MONITOR_INFO ipcMonitorInfo = {
	.cmd = CMD_RUN,
 	.status = STATUS_IDEL,
};

extern unsigned int g_netadd[3];
int DEV_COUNTS = sizeof(dev_socket) / sizeof(SOCKET_INFO);

void *darp_process(void *para);

int main(int argc, char *argv[])
{
	int i, j, ret;
	int thread_counts;
	struct timespec delay_req, delay_rem;
	pthread_attr_t attr[DEV_COUNTS], attr_ipc;
	pthread_t thread_sock[DEV_COUNTS], thread_ipc;
	
        printf("\nVersion:%s ", VERSION);
        printf("Author:%s\n",AUTHOR);
	printf("%s ,compiled at %s %s.\n",argv[0],__DATE__,__TIME__);

	/* deal with zobie process */
	struct sigaction sa; 
	sa.sa_handler = SIG_IGN; 
	sa.sa_flags = SA_NOCLDWAIT; 
	sigemptyset(&sa.sa_mask); 
	sigaction(SIGCHLD, &sa, NULL); 
	
	/* open route table file */
	ret = open_route_file();
	if(0 != ret) {
		printf("Failed to open the route table info file.\n");
		exit(FAIL_TO_OPEN_ROUTE_FILE);
	}

	/* create ipc monitor thread */
	ret = pthread_attr_init(&attr_ipc);
	if(ret != 0) {
		printf("Faile to init ipc thread attribute.\n ");
		ret = ERR_INIT_THREAD_ATTR;
		goto Out;
	}
	ret = pthread_create(&thread_ipc, &attr_ipc, ipc_monitor, &ipcMonitorInfo);
	if(ret !=0 ) {
		printf("Faile to create ipc thread.\n ");
		ret = ERR_CREATE_THREAD_IPC;
		goto Out;
	}
	
	/* create droute process thread for each socket interface */
	for(i=0; i<MAX_DEV_NAMES; i++) {
		ret = pthread_attr_init(&attr[i]);
		if(ret != 0) {
			printf("%s:%d-Faile to init thread attribute.\n ", __FILE__, __LINE__);
			return -3;
		}
#ifdef DARP
		g_netadd[i] = 0;
		ret = pthread_create(&thread_sock[i], &attr[i], darp_process, &dev_socket[i]);
		if(ret != 0) {
			printf("%s:%d-Faile to init thread attribute.\n ", __FILE__, __LINE__);
			return -4;
		}
#else
		dev_socket[i].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL ));
		if(dev_socket[i].sock<0){
			printf("Failed to create socket for %s .\n", dev_socket[i].name);
			ret = ERR_CREATE_SOCKET;
			goto Out;
		}

		ret = pthread_create(&thread_sock[i], &attr[i], packet_monitor, &dev_socket[i]);
		if(ret != 0) {
			printf("%s:%d-Faile to init thread attribute. %d .\n ",__FILE__,__LINE__);
			return -4;
		}
#endif
	}
	
	delay_req.tv_sec = 0;
	delay_req.tv_nsec = 500*1000*1000;

	/* all thread counts */
	thread_counts = DEV_COUNTS + 1;

	/* while loop break if any pthread is dead. */
	while(1) {		
		nanosleep(&delay_req, &delay_rem);
		for(i=0; i<DEV_COUNTS; i++){
			if(dev_socket[i].status == STATUS_DEAD)
				break;
		}
		if(i < DEV_COUNTS)
			break;
		if(ipcMonitorInfo.status == STATUS_DEAD)
			break;
	}


	/* exit command for every thread. */
	for(i=0; i<DEV_COUNTS; i++)
		dev_socket[i].cmd = CMD_EXIT;
	ipcMonitorInfo.cmd = CMD_EXIT;

	/* while loop break if all pthread are dead */
	while(1){
		nanosleep(&delay_req, &delay_rem);
		i = 0;
		for(j=0; j<DEV_COUNTS; j++){
			if(dev_socket[j].status == STATUS_DEAD)
				i++;
		}
		if(ipcMonitorInfo.status == STATUS_DEAD)
			i++;
		if(i < thread_counts)
			continue;
		else
			break;
	}
Out:
	close_route_file();		

	for(i=0; i<DEV_COUNTS; i++){
		if(dev_socket[i].sock != -1){
			close(dev_socket[i].sock);
			dev_socket[i].sock = -1;
		}
	}

	return ret;
}

