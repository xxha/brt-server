/* The api interface for bind route */
#ifndef __BIND_ROUTE_H
#define __BIND_ROUTE_H
#ifdef __cplusplus
extern "C" {
#endif

#define BIND_ROUTE_ON 1

typedef enum {
	BIND_SUCCESS=0,
	BIND_KEY_FILE_NOT_EXIST,
	BIND_FAIL_TO_GET_KEY,
	BIND_FAIL_TO_GET_MSG_ID,
	BIND_FAIL_SND_MESSAGE,
	BIND_FAIL_RCV_MESSAGE,
	BIND_FAIL_TO_BIND_ROUTE
}BIND_RT_REVALUE;

typedef struct {
	BIND_RT_REVALUE val;
	char errInfo[128];
}BIND_RET_INFO;

typedef enum {
	DEV_MAP_SUCCESS=0,
	DEV_MAP_KEY_FILE_NOT_EXIST,
	DEV_MAP_FAIL_TO_GET_KEY,
	DEV_MAP_FAIL_TO_GET_MSG_ID,
	DEV_MAP_FAIL_SND_MESSAGE,
	DEV_MAP_FAIL_RCV_MESSAGE,
	DEV_MAP_FAIL_TO_MAP_NET_DEV,
}NET_DEV_NAME_RE_VALUE;

typedef struct {
	NET_DEV_NAME_RE_VALUE val;
	unsigned char errInfo[128];
}NET_DEV_NAME_RET_INFO;

typedef enum {
	SET_DEV_NET_INFO_SUCCESS=0,
	SET_DEV_NET_INFO_KEY_FILE_NOT_EXIST,
	SET_DEV_NET_INFO_FAIL_TO_GET_KEY,
	SET_DEV_NET_INFO_FAIL_TO_GET_MSG_ID,
	SET_DEV_NET_INFO_FAIL_SND_MESSAGE,
	SET_DEV_NET_INFO_FAIL_RCV_MESSAGE,
	SET_DEV_NET_INFO_FAIL_TO_SET_NET_INFO,
}NET_DEV_INFO_RE_VALUE;

typedef struct {
	NET_DEV_INFO_RE_VALUE val;
	unsigned char errInfo[128];
}NET_DEV_INFO_RET_INFO;

typedef struct {
	char orgDevName[32];
	char newDevName[32];
}NET_DEV_NAME_MAP_NODE;

//gateway infomation
typedef struct {
	char dev_name[32];
	char gateway[32];
	char status[32];
	char netmask[32];
	char ip[32];
	char mac[32];
	char work;// 0,sleep ,1 work.
}DEV_NET_INFO;

NET_DEV_INFO_RET_INFO v100p_set_net_info(DEV_NET_INFO *devNetInfo);
NET_DEV_NAME_RET_INFO v100p_net_name_map(NET_DEV_NAME_MAP_NODE *list,int counts);
BIND_RET_INFO v100p_bind_route(char *dst_addr, char *net_mask, char *gw, char *dev_name);

#ifdef __cplusplus
}
#endif
#endif

