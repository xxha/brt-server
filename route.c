#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pub.h"
#include "route.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ROUTE_BUF_SIZE 		1024
#define ROUTE_FILE_NAME 	"/proc/net/route"
#define ADD_HOST_ROUTE 		"route add -host "
#define ADD_HOST_ROUTE_NP 	"np route add "
#define REMOVE_HOST_ROUTE 	"route del -host "
#define ADD_DEFUALT_GW 		"route add -net 0.0.0.0 netmask 0.0.0.0 dev "
#define DEL_DEFUALT_GW 		"route del -net 0.0.0.0 >/dev/null 2>&1 "
#define EXCEPT_DST_ADR 		"0.0.0.0"
#define FLUSH_ROUTE_CACHE 	"np route flush cache"
#define HIDE_CMD_INFO 		">/dev/null 2>&1"


int open_route_file();
void close_route_file();
static int broadcast(unsigned char *dst_addr,unsigned char *net_mask,unsigned char *gw,unsigned char *dev_name);

static FILE *routeFile=NULL;
static unsigned char routeBuf[ROUTE_BUF_SIZE+1];
static pthread_mutex_t rt_table_mutex=PTHREAD_MUTEX_INITIALIZER;

typedef struct{
	unsigned char eth_name[16];
	unsigned char dot_ip[18];
	unsigned char gw[18];
	unsigned char flags;
	unsigned char refCnt;
	unsigned char use;
	unsigned char Metric;
	unsigned char mask;
	int mtu;
	int window;
	int irtt;
}ROUTE_TABLE;

/*Add new route*/

/*Delete the old one if it is needed.*/
inline int keep_route(unsigned char *dst_addr,unsigned char *net_mask,unsigned char *gw,unsigned char *dev_name,unsigned char *host_ip){

	int ret,i;
	char *p,*q,*head,*tail;
	ROUTE_TABLE rt;
	unsigned char tmp[4],cmd[128],strIP[4];
	int add=0;
	struct in_addr inpIP,inpGW;
	unsigned int netmask;
	unsigned char rt_gateway[16];
	int add_new_host_route;
	char local_gw[32];
	unsigned int dstip,hostip,mask;

	//printf("dst_addr:%s  net_mask:%s gateway:%s dev_name:%s host ip:%s\n",dst_addr,net_mask,gw,dev_name,host_ip);
	
	/* deal with exception */
	if((strcmp(EXCEPT_DST_ADR,dst_addr)==0)||(!isdigit(dst_addr[0]))){
		//printf("%s:%d dst_addr:%s\n",__FILE__,__LINE__,dst_addr);
		return -1;
	}
	strcpy(local_gw,gw);
	/*dealwith broad cast */
	inet_aton(dst_addr, &inpIP);
	//printf("%s %08x\n",dst_addr,inpIP.s_addr);
	if((inpIP.s_addr&0xFF000000)==0xFF000000){
		printf("%s:%d\n",__FILE__,__LINE__);
		//pthread_mutex_lock(&rt_table_mutex);	
		ret=broadcast(dst_addr,net_mask,local_gw,dev_name);
		//pthread_mutex_unlock(&rt_table_mutex);			
		return 0;
	}

	sprintf(cmd,"np route show %s",dst_addr);
	FILE *pipeFile=popen(cmd,"r");
	if(NULL==pipeFile){
		printf("Failed to exucte the command.\n");
		return 0;
	}

	add_new_host_route=0;
	while(1){
		p=fgets(routeBuf,ROUTE_BUF_SIZE,pipeFile);
		if(NULL==p){
			add_new_host_route++;
			break;
		}
		//printf("read a line:%s\n",p);
		q=strstr(p,dev_name);
		if(NULL==q){//ambiguous ,delete it.
			sprintf(cmd,"np route del %s 2>&1",p);
			system(cmd);
			add_new_host_route++;
			continue;
		}
		//ip same ,dev same ,compare the gateway.
		if(strcmp(local_gw,"0.0.0.0")==0){
			add_new_host_route--;//the old one have more fluent infomation than the new one .
			continue;
		}
		q=strstr(p,"via");
		if(NULL==q){
			add_new_host_route++;
			continue;//the old route item have no gateway info. add the new route anymore.
		}else{
			head=q;
			while(*head==0x20)
				head++;
			tail=head;
			while(*tail!=0x20)
				tail++;
			memcpy(rt_gateway,head,tail-head);
			rt_gateway[tail-head]=0x00;			
		}
		if(strcmp(rt_gateway,local_gw)==0){
			add_new_host_route--;//they are not empty, but are equal. so leave it alone .
			continue;
		}else{
			//the new one is fluent than the old one ,delete the old one.
			sprintf(cmd,"np route del %s 2>&1",p);
			system(cmd);
			add_new_host_route++;
			continue;
		}
			
			
	}
	if(add_new_host_route>0){
		//printf("%s:%d\n",__FILE__,__LINE__);
		// compare the dest ip and the host ip. same subnet or different subnet.
		ret=inet_pton(AF_INET,dst_addr,&dstip);
		if(ret!=1){
			pclose(pipeFile);
			return -1;
		}
		//printf("%s:%d\n",__FILE__,__LINE__);
		ret=inet_pton(AF_INET,host_ip,&hostip);
		if(ret!=1){
			pclose(pipeFile);
			return -1;
		}
		//printf("%s:%d\n",__FILE__,__LINE__);
		ret=inet_pton(AF_INET,net_mask,&mask);
		if(ret!=1){
			pclose(pipeFile);
			return -1;
		}
		//printf("%s:%d dst_ip:%08x net mask:%08x host ip:%08x \n",__FILE__,__LINE__,dstip,mask,hostip);
		//printf("%08x %08x\n",dstip&mask,hostip&mask);
		if((dstip&mask)==(hostip&mask)){//same subnet.
			//printf("%s:%d\n",__FILE__,__LINE__);
			strcpy(local_gw,"0.0.0.0");
		}else{
			//printf("%s:%d\n",__FILE__,__LINE__);
			if(strcmp(local_gw,"0.0.0.0")==0){
				pclose(pipeFile);
				return ;// different subnet ,need to have gateway.
			}
		}
		sprintf(cmd,"np route add %s via %s dev %s  %s",dst_addr,local_gw,dev_name,HIDE_CMD_INFO);		
		system(cmd);
	}

	pclose(pipeFile);
}

static int broadcast(unsigned char *dst_addr,unsigned char *net_mask,unsigned char *gw,unsigned char *dev_name){

	unsigned char cmd[128];
	
	memset(cmd,0x00,sizeof(cmd));
	sprintf(cmd,"%s %s >/dev/null 2>&1","np route del table local broadcast ",dst_addr);
#if 0
	printf("%s\n",cmd);
#endif
	system(cmd);
	system(cmd);

	memset(cmd,0x00,sizeof(cmd));
	sprintf(cmd,"%s %s %s %s >/dev/null 2>&1","np route add table local broadcast ",dst_addr,"dev",dev_name);
#if 0
	printf("%s\n",cmd);
#endif
	
	system(cmd);
	sleep(3);//fixme in the future,it need some delay to work fine.I don't know why.
	return 0;
}

/*0,success;-1,failed.*/
int open_route_file()
{
	if(NULL == routeFile) {
		routeFile = fopen(ROUTE_FILE_NAME,"r");		
		if(routeFile == NULL)
			return -1;
		else
			return 0;
	}
}
void close_route_file()
{
	if(routeFile != NULL){	
		fclose(routeFile);
		routeFile = NULL;
	}
}
