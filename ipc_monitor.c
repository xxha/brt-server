
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//ipc 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#include "msgque.h"
#include "main.h"
#include "pub.h"
#include "bindrt.h"

#define FLUSH_ROUTE_CACHE "ip route flush cache"
extern int DEV_COUNTS;
extern SOCKET_INFO dev_socket[MAX_DEV_NAMES];
/* communicate with the other process */
void *ipc_monitor(void *para){
	struct msgtype msg; 
    key_t key_c2s,key_s2c; 
    int msgid_c2s,msgid_s2c;
    int ret;
    struct timespec delay_req,delay_rem;
    BIND_ROUTE_MSG_INFO_BODY *msgBodyBindRoute;
	NET_DEV_NAME_MAP_MSG_INFO_BODY *msgBodyNetDevMap;
	DEV_NET_INFO *msgDevNetInfo;
	unsigned char dev_name_map[128];
    int i,j;

	IPC_MONITOR_INFO *ipcMonitorPtr;
	
	if(para==NULL){//It should not read here ,only occur in the debug steps.
		printf("One can't make bricks without straw :).\n");
		exit(FAIL_ILLEGAL_PARA_FOR_THREAD);
	}
	ipcMonitorPtr=(IPC_MONITOR_INFO *)para;
	ipcMonitorPtr->status=STATUS_RUN;


#if 0
	if(access(UNIQ_FILE_NAME_C2S,F_OK)){
		ret=system("touch "UNIQ_FILE_NAME_C2S);
		if(-1==ret){
			printf("Failed to create the unique file %s.\n",UNIQ_FILE_NAME_C2S);
			ipcMonitorPtr->status=STATUS_DEAD;
			ret=-1;
			pthread_exit(&ret);

		}
	}
	if(access(UNIQ_FILE_NAME_S2C,F_OK)){
		ret=system("touch "UNIQ_FILE_NAME_S2C);
		if(-1==ret){
			printf("Failed to create the unique file %s.\n",UNIQ_FILE_NAME_S2C);
			ipcMonitorPtr->status=STATUS_DEAD;
			ret=-1;
			pthread_exit(&ret);
		}
	}
#endif
#if 0
	ret=system("rm -fr "UNIQ_FILE_NAME_S2C);
	if(-1==ret){
		printf("Failed delete the unique file %s.\n",UNIQ_FILE_NAME_S2C);
		exit(FAIL_TO_DEL_UNIQ_QUE_FILE);
	}
	ret=system("rm -fr "UNIQ_FILE_NAME_C2S);
	if(-1==ret){
		printf("Failed delete the unique file %s.\n",UNIQ_FILE_NAME_C2S);
		exit(FAIL_TO_DEL_UNIQ_QUE_FILE);
	}
	
	ret=system("touch "UNIQ_FILE_NAME_C2S);
	if(-1==ret){
		printf("Failed to create the unique file.\n");
		exit(FAIL_TO_CREATE_UNIQ_QUE_FILE);
	}
#endif	
	key_c2s=ftok(KEY_FILE_NAME,1);
	if(key_c2s==-1){
		printf("Failed to get key of msq quee.\n");
		ipcMonitorPtr->status=STATUS_DEAD;
		ret=-1;
		pthread_exit(&ret);
	}
	
	key_s2c=ftok(KEY_FILE_NAME,2);
	if(key_s2c==-1){
		printf("Failed to get key of msq quee.\n");
		ipcMonitorPtr->status=STATUS_DEAD;
		ret=-1;
		pthread_exit(&ret);
	}

	msgid_c2s=msgget(key_c2s, IPC_CREAT);
    if(msgid_c2s==-1){
    	perror(NULL);
		printf("Failed to get msg queue id.\n");
		ipcMonitorPtr->status=STATUS_DEAD;
		ret=-1;
		pthread_exit(&ret);
    }
    msgid_s2c=msgget(key_s2c,IPC_CREAT);
    if(msgid_s2c==-1){
		perror(NULL);
		printf("Failed to get msg queue id.\n");
		ipcMonitorPtr->status=STATUS_DEAD;
		ret=-1;
		pthread_exit(&ret);
    }    
    printf("msgid_s2c = %d msgid_c2s:%d \n", msgid_s2c,msgid_c2s);
    msgBodyBindRoute=(BIND_ROUTE_MSG_INFO_BODY *)msg.buf;
    msgBodyNetDevMap=(NET_DEV_NAME_MAP_MSG_INFO_BODY *)msg.buf;
	msgDevNetInfo=(DEV_NET_INFO *)msg.buf;
	
    delay_req.tv_sec=0;
	delay_req.tv_nsec=500*1000*1000;


    while(1)
    { 
    	nanosleep(&delay_req,&delay_rem);
 	
    	memset(msg.buf,0x00,sizeof(msg.buf));
    	msg.mtype=0x00;
        ret=msgrcv(msgid_c2s, &msg, sizeof(struct msgtype),0,0);
        if(ret==-1){
        	//perror("Server:");
        	//printf("Failed to receiv  msg .\n");
        	//ret=-3;
        	//pthread_exit(&ret);
        	continue;
        }
        switch (msg.mtype){
			case TYPE_BIND_ROUTE:
				printf("Server Receive:%s %s %s\n", msgBodyBindRoute->ip_addr,msgBodyBindRoute->gw,msgBodyBindRoute->dev_name); 
				if(strcmp(msgBodyBindRoute->gw,"*")==0)
					strcpy(msgBodyBindRoute->gw,"0.0.0.0");
	        	ret=keep_route(msgBodyBindRoute->ip_addr,msgBodyBindRoute->net_mask,msgBodyBindRoute->gw,msgBodyBindRoute->dev_name,msgBodyBindRoute->gw);
	        	//printf("%s:%d\n",__FILE__,__LINE__);
	        	if(ret==0){
		            strcpy(msgBodyBindRoute->status,"Success");
		            system(FLUSH_ROUTE_CACHE);//fixme for better performance.
	        	}
	        	else{
					strcpy(msgBodyBindRoute->status,"Fail");
				}
	        	//printf("%s:%d\n",__FILE__,__LINE__);

				msg.mtype =TYPE_BIND_ROUTE;
		        ret=msgsnd(msgid_s2c, &msg, sizeof(struct msgtype), 0);
		        if(ret==-1){
		        	perror(NULL);
		        	printf("%s:%d Failed to send  msg .\n",__FILE__,__LINE__);
		        	//ret=-4;
		        	//pthread_exit(&ret);
		        	;
		        }
	        	//printf("%s:%d\n",__FILE__,__LINE__);
		        
				break;

			case TYPE_DEV_NAME_MAP:	
				ret=strtol(msgBodyNetDevMap->counts,NULL,0);
#if 0       	
	        	printf("counts:%s\n",msgBodyNetDevMap->counts);
	        	
	        	for(i=0;i<ret;i++){
					printf("Dev org name:%s  Dev new name:%s\n",msgBodyNetDevMap->net_dev_name_map_list[i].orgDevName,msgBodyNetDevMap->net_dev_name_map_list[i].newDevName);								
	        	}
#endif        	
#if 0
				printf("Before.\n");
				for(i=0;i<DEV_COUNTS;i++)
					printf("%s %s\n",dev_socket[i].name,dev_socket[i].alias);
#endif					
				for(i=0;i<ret;i++){
					for(j=0;j<DEV_COUNTS;j++){						
						if(strcmp(dev_socket[j].name,msgBodyNetDevMap->net_dev_name_map_list[i].orgDevName)==0){
							strcpy(dev_socket[j].alias,msgBodyNetDevMap->net_dev_name_map_list[i].newDevName);
							dev_socket[j].update=1;	
							printf("%d %s %s\n",dev_socket[j].update,dev_socket[j].name,dev_socket[j].alias);
							ret=bind_socket_to_devname(&dev_socket[j]);

							//check the return value in the future.
							}
					}						
				}	
				
#if 0
				printf("After.\n");
			
				for(i=0;i<DEV_COUNTS;i++)
					printf("%s %s\n",dev_socket[i].name,dev_socket[i].alias);
#endif
				//update the netdevice table.
				strcpy(msgBodyNetDevMap->status,"Success");
				msg.mtype =TYPE_DEV_NAME_MAP;
		        ret=msgsnd(msgid_s2c, &msg, sizeof(struct msgtype), 0);
		        if(ret==-1){
		        	perror(NULL);
		        	printf("%s:%d Failed to send  msg .\n",__FILE__,__LINE__);
		        	//ret=-4;
		        	//pthread_exit(&ret);
		        }
				break;
			case TYPE_DEV_INFO:
				printf("%s:%d-dev name:%s gateway:%s\n",__FILE__,__LINE__,msgDevNetInfo->dev_name,msgDevNetInfo->gateway);
				strcpy(msgDevNetInfo->status,"Fail");
				for(i=0;i<DEV_COUNTS;i++){
					if(strcmp(msgDevNetInfo->dev_name,dev_socket[i].name)==0){
						strcpy(dev_socket[i].gateway,msgDevNetInfo->gateway);
						strcpy(msgDevNetInfo->status,"Success");
						break;
					}
				}
				msg.mtype =TYPE_DEV_INFO;
				printf("%s\n",msgDevNetInfo->status);
		        ret=msgsnd(msgid_s2c, &msg, sizeof(struct msgtype), 0);
		        if(ret==-1){
		        	perror(NULL);
		        	printf("%s:%d Failed to send  msg .\n",__FILE__,__LINE__);
		        	//ret=-4;
		        	//pthread_exit(&ret);
		        }
		        break;
			default:
				break;
        }
    } 
}

