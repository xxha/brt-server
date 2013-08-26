#include <stdio.h>
#include <string.h>

#include "main.h"
#include "scan_dev_name.h"

static DEV_NAME_MAP devNameMap[MAX_DEV_ORG_COUNTS]={
	{
		.dev_org_name="eth0",
		.dev_cur_name="eth0",
		.dev_new_name="eth0",
	},
	{
		.dev_org_name="eth1",
		.dev_cur_name="eth1",
		.dev_new_name="eth1",
	},
	{
		.dev_org_name="eth3",
		.dev_cur_name="eth3",
		.dev_new_name="eth3",
	},
};

#define DEV_PROC_FILE_NAME "/proc/net/dev"
void *scan_net_dev_name(void *para){

	FILE *devNameFile;
	unsigned char buf[512],tmp[32];
	int ret;
	unsigned char *p;
	unsigned char *tail,*head;
	int i,j;
	
	while(1){
		memset(devNameMap,0x00,MAX_DEV_NAMES*sizeof(DEV_NAME_MAP));
		i=0;
		devNameFile=fopen(DEV_PROC_FILE_NAME,"r");
		if(devNameFile==NULL){
			printf("%s:%d-Failed to open the net device name file %s .\n",__FILE__,__LINE__,DEV_PROC_FILE_NAME);
			exit(-1);
		}
		//skip the first line.
		p=fread(buf,sizeof(buf),devNameFile);
		
		while(1){
			p=fread(buf,sizeof(buf),devNameFile);
			if(NULL==p)
				break;//reach the end of the file.
			for(i=0;i<MAX_DEV_ORG_COUNTS;i++){
				head=strstr(buf,devNameMap[i].dev_org_name);
				if(head!=NULL){
					tail=strstr(buf,":");
					if(tail==NULL)
						break;//break out the for cycle.
					strcpy(tmp,head,tail-head);
					
				}
			}
			head=strstr(p,"eth");
			if(NULL==head)
				continue;
			tail=head;
			while(*tail!=':')
				tail++;
			memncpy(devNameMap[i].dev_name,head,4);//Fix me if the net device is more than 9
			memncpy(devNameMap[i].dev_name,head,tail-head);
			i++;
		}
		fclose(devNameFIle);

		for(i=0;i<MAX_DEV_NAMES;i++){
			for(j=0;j<MAX_DEV_NAMES;j++)
			if(strcmp(
		}
		
	
	





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

	delay_req.tv_sec=0;
	delay_req.tv_nsec=200*1000*1000;//300ms
	
	len=sizeof(peerSockAddr);  
	while(1){
		if(sockPtr->cmd==CMD_EXIT){
			sockPtr->status=STATUS_DEAD;
			ret=-1;
			pthread_exit(&ret);
		}
		
		//sleep(1);
#if 1	
		nanosleep(&delay_req,&delay_rem);
		//printf("cmd=%s ; filename=%s\n",sockPtr->cmd,sockPtr->fileName);
		sprintf(fileName,"%s%s%s",PROC_FILE_NAME_HEAD,sockPtr->alias,PROC_FILE_NAME_TAIL);
		//printf("%s :%s :%s:%d\n",sockPtr->name,sockPtr->alias,fileName,sockPtr->update);
		if(access(fileName,F_OK)){
				//printf("%s:%s\n",cmd,fileName);
				eth_dev_appear=0;
				continue;
		}
		
		/* check if the device name has been changed */
		if(sockPtr->update==1){
			printf("%s:%d\n",__FILE__,__LINE__);
			sockPtr->update=0;
			eth_dev_appear=0;
		}
			
		//printf("%s:%d\n",__FILE__,__LINE__);
		
		if(eth_dev_appear==0){
			sprintf(cmd,"%s%s%s",SYS_CTL_HEAD,sockPtr->alias,SYS_CTL_TAIL);
			printf("%s\n",cmd);
			system(cmd);
			/*bind socket to device*/
			strcpy(ifr.ifr_name,sockPtr->alias);		
			ret=ioctl(sockPtr->sock,SIOCGIFINDEX,&ifr);
			if(ret<0){
				printf("%s:%d Failed to retrive the interface index of %s\n ",__FILE__,__LINE__,ifr.ifr_name);
				continue;//maybe the device disappler now, try for the next times.
				//goto Out2;
			}
			l2.sll_family=AF_PACKET;
			l2.sll_protocol=htons(ETH_P_ALL);
			l2.sll_ifindex=ifr.ifr_ifindex;		
			//printf("%d\n",ifr[i].ifr_ifindex);
			ret=bind(sockPtr->sock,(struct sockaddr *)&l2,sizeof(l2));
			if(ret<0){
				printf("Failed to bind %s to socket %d.\n",sockPtr->alias,sockPtr->sock);
				continue;//
				//goto Out2;
			}
			//get the mac address.
			strcpy(ifr.ifr_name,sockPtr->alias);
		  	ret=ioctl(sockPtr->sock,SIOCGIFHWADDR,&ifr);
		  	if(ret<0){
		  		printf("Failed to get mac address.\n");
		  		continue;
		  	}
		  	printf("Mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[0],
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[1],
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[2],
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[3],
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[4],
			  	(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
			sockPtr->mac[0]=(unsigned char)ifr.ifr_hwaddr.sa_data[0];
			sockPtr->mac[1]=(unsigned char)ifr.ifr_hwaddr.sa_data[1];
			sockPtr->mac[2]=(unsigned char)ifr.ifr_hwaddr.sa_data[2];			
			sockPtr->mac[3]=(unsigned char)ifr.ifr_hwaddr.sa_data[3];			
			sockPtr->mac[4]=(unsigned char)ifr.ifr_hwaddr.sa_data[4];			
			sockPtr->mac[5]=(unsigned char)ifr.ifr_hwaddr.sa_data[5];			

			/*get my ip address*/
			strcpy(ifr.ifr_name,sockPtr->alias);
			printf("%s\n",ifr.ifr_name);
			ret=ioctl(sockPtr->sock,SIOCGIFADDR,&ifr);
			if(ret<0){
				perror(NULL);
				printf("%s:%d\n",__FILE__,__LINE__);
				//printf("Failed to retrive the interface ip address %s\n ",sockPtr->alias);
				continue;
			}
		
			sockPtr->ip_addr=(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
			eth_dev_appear=1;
		}
#endif
		
		//printf("%s addr:%08x\n",sockPtr->alias,sockPtr->ip_addr);
		memset(buf,0x00,sizeof(buf));
		ret=recvfrom(sockPtr->sock,buf,MAX_FRAME_SIZE,0,(struct sockaddr *)&peerSockAddr,&len);
		//ret=recvfrom(sockPtr->sock,buf,MAX_FRAME_SIZE,MSG_DONTWAIT,(struct sockaddr *)&peerSockAddr,&len);
	 	if(ret == -1){
	 		//perror(NULL);
	    	//printf("Error occur when reading data from %s socket\n",sockPtr->name);
	    	//ret=ERR_RCV_SOCK_DATA;
			//pthread_exit(&ret);
			continue;
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
		if((buf[6]!=0x00)||
			(buf[7]!=0x16)||
			(buf[8]!=0x76)||
			(buf[9]!=0xb7)||
			(buf[10]!=0x6f)||
			(buf[11]!=0xC0))
			continue;
			
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


}
