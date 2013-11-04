#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bindrt.h"
#include "msgque.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
 
static int msgid_c2s = 0,msgid_s2c = 0; 
static volatile int init_msg_que = 0;
static key_t key_c2s,key_s2c; 

BIND_RET_INFO bindRtInfo[] = {
	{
		.val = BIND_SUCCESS,
		.errInfo = "Bind success",
	},
	{
		.val = BIND_KEY_FILE_NOT_EXIST,
		.errInfo = "The key file doesn't exist",
	},
	{
		.val = BIND_FAIL_TO_GET_KEY,
		.errInfo = "Failed to get key from file name",
	},
	{
		.val = BIND_FAIL_TO_GET_MSG_ID,
		.errInfo = "Failed to get msg id from key",
	},
	{	
		.val = BIND_FAIL_SND_MESSAGE,
		.errInfo = "Failed to send msg to the server",
	},
	{
		.val = BIND_FAIL_RCV_MESSAGE,
		.errInfo = "Failed to receive msg from the server",
	},	
	{
		.val = BIND_FAIL_TO_BIND_ROUTE,
		.errInfo = "Failed to bind the route",
	},
};

NET_DEV_NAME_RET_INFO netDevNameInfo[] = {
	{	
		.val = DEV_MAP_SUCCESS,
		.errInfo = "Net device name map success.",
	},
	{
		.val = BIND_KEY_FILE_NOT_EXIST,
		.errInfo = "The key file doesn't exist",
	},
	{
		.val = BIND_FAIL_TO_GET_KEY,
		.errInfo = "Failed to get key from file name",
	},
	{
		.val = BIND_FAIL_TO_GET_MSG_ID,
		.errInfo = "Failed to get msg id from key",
	},
	{	
		.val = BIND_FAIL_SND_MESSAGE,
		.errInfo = "Failed to send msg to the server",
	},
	{
		.val = BIND_FAIL_RCV_MESSAGE,
		.errInfo = "Failed to receive msg from the server",
	},	
	{
		.val = BIND_FAIL_TO_BIND_ROUTE,
		.errInfo = "Failed to bind the route",
	},

};

NET_DEV_INFO_RET_INFO devNetInfo[] = {
	{	
		.val = SET_DEV_NET_INFO_SUCCESS,
		.errInfo = "Set net device info success.",
	},
	{
		.val = BIND_KEY_FILE_NOT_EXIST,
		.errInfo = "The key file doesn't exist",
	},
	{
		.val = BIND_FAIL_TO_GET_KEY,
		.errInfo = "Failed to get key from file name",
	},
	{
		.val = BIND_FAIL_TO_GET_MSG_ID,
		.errInfo = "Failed to get msg id from key",
	},
	{	
		.val = BIND_FAIL_SND_MESSAGE,
		.errInfo = "Failed to send msg to the server",
	},
	{
		.val = BIND_FAIL_RCV_MESSAGE,
		.errInfo = "Failed to receive msg from the server",
	},	
	{
		.val = SET_DEV_NET_INFO_FAIL_TO_SET_NET_INFO,
		.errInfo = "Failed to bind the route",
	},
};

BIND_RET_INFO v100p_bind_route(char *dst_addr, char *net_mask, char *gw, char *dev_name)
{
	int ret;
	struct msgtype msg;
	BIND_ROUTE_MSG_INFO_BODY *msgBody;

#ifndef BIND_ROUTE_ON	
	return bindRtInfo[DEV_MAP_SUCCESS];
#endif

	/* init msg quee*/
	if(init_msg_que == 0){
		key_c2s = ftok(KEY_FILE_NAME, 1);
		if(key_c2s == -1)
			return bindRtInfo[BIND_FAIL_TO_GET_KEY];

		key_s2c = ftok(KEY_FILE_NAME, 2);
		if(key_s2c == -1)
			return bindRtInfo[BIND_FAIL_TO_GET_KEY];

		msgid_c2s = msgget(key_c2s, PERM);
		if(msgid_c2s == -1)
			return bindRtInfo[BIND_FAIL_TO_GET_MSG_ID];

		msgid_s2c = msgget(key_s2c, PERM);
		if(msgid_s2c == -1)
			return bindRtInfo[BIND_FAIL_TO_GET_MSG_ID];

		init_msg_que = 1;
	}

	msg.mtype = TYPE_BIND_ROUTE;
	memset(msg.buf, 0x00, sizeof(msg.buf));
	msgBody = (BIND_ROUTE_MSG_INFO_BODY *)msg.buf;
	strcpy(msgBody->ip_addr, dst_addr);
	strcpy(msgBody->net_mask, net_mask);
	strcpy(msgBody->gw, gw);
	strcpy(msgBody->dev_name, dev_name);

	ret = msgsnd(msgid_c2s, &msg, sizeof(struct msgtype), 0);
	if(ret == -1){
		perror("Clinet:");
		return bindRtInfo[BIND_FAIL_SND_MESSAGE];
	}
		
	ret = msgrcv(msgid_s2c, &msg, sizeof(struct msgtype), 0, 0); 
	if(ret == -1)
		return bindRtInfo[BIND_FAIL_RCV_MESSAGE];

	if(strcmp(msgBody->status, "Success") != 0)
		return bindRtInfo[BIND_FAIL_TO_BIND_ROUTE];	

	return bindRtInfo[BIND_SUCCESS];	
}

NET_DEV_NAME_RET_INFO v100p_net_name_map(NET_DEV_NAME_MAP_NODE *list, int counts)
{
	int ret,i;
	struct msgtype msg;
	NET_DEV_NAME_MAP_MSG_INFO_BODY *msgBody;

#ifndef BIND_ROUTE_ON	
	return netDevNameInfo[DEV_MAP_SUCCESS];
#endif
	/* init msg quee*/
	if(init_msg_que == 0) {
		key_c2s = ftok(KEY_FILE_NAME, 1);
		if(key_c2s == -1)
			return netDevNameInfo[DEV_MAP_FAIL_TO_GET_KEY];

		key_s2c = ftok(KEY_FILE_NAME, 2);
		if(key_s2c == -1)
			return netDevNameInfo[DEV_MAP_FAIL_TO_GET_KEY];

		msgid_c2s = msgget(key_c2s, PERM);
		if(msgid_c2s == -1)
			return netDevNameInfo[DEV_MAP_FAIL_TO_GET_MSG_ID];

		msgid_s2c = msgget(key_s2c, PERM);
		if(msgid_s2c == -1)
			return netDevNameInfo[DEV_MAP_FAIL_TO_GET_MSG_ID];

		init_msg_que = 1;
	}

	msg.mtype = TYPE_DEV_NAME_MAP;
	memset(msg.buf, 0x00, sizeof(msg.buf));
	msgBody = (NET_DEV_NAME_MAP_MSG_INFO_BODY *)msg.buf;
	sprintf(msgBody->counts, "%d", counts);

	for(i=0; i<counts; i++){
		strcpy(msgBody->net_dev_name_map_list[i].orgDevName, list[i].orgDevName);
		strcpy(msgBody->net_dev_name_map_list[i].newDevName, list[i].newDevName);
	}

	ret = msgsnd(msgid_c2s, &msg, sizeof(struct msgtype), 0);
	if(ret == -1){
		perror("Clinet:");
		return netDevNameInfo[DEV_MAP_FAIL_SND_MESSAGE];
	}

	ret = msgrcv(msgid_s2c, &msg, sizeof(struct msgtype), TYPE_DEV_NAME_MAP, 0); 
	if(ret == -1)
		return netDevNameInfo[DEV_MAP_FAIL_RCV_MESSAGE];

	if(strcmp(msgBody->status, "Success")!=0)		
		return netDevNameInfo[DEV_MAP_FAIL_TO_MAP_NET_DEV];	

	return netDevNameInfo[DEV_MAP_SUCCESS];		
}

NET_DEV_INFO_RET_INFO v100p_set_net_info(DEV_NET_INFO *devNetInfoPtr)
{
	int ret;
	struct msgtype msg;
	DEV_NET_INFO *msgBody;

#ifndef BIND_ROUTE_ON	
	return devNetInfo[DEV_MAP_SUCCESS];
#endif
	/* init msg quee*/
	if(init_msg_que == 0){
		ret = access(KEY_FILE_NAME, F_OK);
		if(ret != 0)
			return devNetInfo[DEV_MAP_KEY_FILE_NOT_EXIST];

		ret = access(KEY_FILE_NAME, F_OK);
		if(ret!=0)
			return devNetInfo[DEV_MAP_KEY_FILE_NOT_EXIST];

		key_c2s = ftok(KEY_FILE_NAME, 1);
		if(key_c2s == -1)
			return devNetInfo[DEV_MAP_FAIL_TO_GET_KEY];

		key_s2c = ftok(KEY_FILE_NAME, 2);
		if(key_s2c == -1)
			return devNetInfo[DEV_MAP_FAIL_TO_GET_KEY];

		msgid_c2s = msgget(key_c2s, PERM);
		if(msgid_c2s == -1)
			return devNetInfo[DEV_MAP_FAIL_TO_GET_MSG_ID];

		msgid_s2c = msgget(key_s2c, PERM);
		if(msgid_s2c == -1)
			return devNetInfo[DEV_MAP_FAIL_TO_GET_MSG_ID];

		init_msg_que = 1;
	}
	msg.mtype = TYPE_DEV_INFO;
	

	memset(msg.buf, 0x00, sizeof(msg.buf));
	msgBody = (DEV_NET_INFO *)msg.buf;

	strcpy(msgBody->dev_name, devNetInfoPtr->dev_name);
	strcpy(msgBody->gateway, devNetInfoPtr->gateway);
	strcpy(msgBody->ip, devNetInfoPtr->ip);
	strcpy(msgBody->netmask, devNetInfoPtr->netmask);
	memcpy(msgBody->mac, devNetInfoPtr->mac, 6);
	msgBody->work = devNetInfoPtr->work;

	printf("%s:%d  dev_name:%s gateway:%s\n", __FILE__, __LINE__,
		msgBody->dev_name, msgBody->gateway);
	printf("%s:%d  ip:%s netmask:%s\n", __FILE__, __LINE__, 
		msgBody->ip, msgBody->netmask);
	printf("%s:%d  mac: %02x-%02x-%02x-%02x-%02x-%02x\n", __FILE__, __LINE__,
		msgBody->mac[0], msgBody->mac[1], msgBody->mac[2],
		msgBody->mac[3], msgBody->mac[4], msgBody->mac[5]);	
	printf("%s:%d  work:%d \n", __FILE__, __LINE__, msgBody->work);

	ret = msgsnd(msgid_c2s, &msg, sizeof(struct msgtype),0);
	if(ret == -1) {
		perror("Clinet:");
		return devNetInfo[DEV_MAP_FAIL_SND_MESSAGE];
	}

	ret = msgrcv(msgid_s2c, &msg, sizeof(struct msgtype), 0, 0); 
	if(ret == -1) {
		perror(NULL);
		return devNetInfo[SET_DEV_NET_INFO_FAIL_RCV_MESSAGE];
	}
	if(strcmp(msgBody->status, "Success") != 0)		
		return devNetInfo[SET_DEV_NET_INFO_FAIL_TO_SET_NET_INFO];	

	return devNetInfo[SET_DEV_NET_INFO_SUCCESS];		
}


