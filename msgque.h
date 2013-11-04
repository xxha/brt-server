#ifndef __MSQ__QUE__H
#define __MSG__QUE__H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define   BUFFER 		255
#define   MAX_DEV_NAME_LEN 	8
#define   MAX_TXT_BUFFER_LEN 	64
#define   PERM 			S_IRUSR|S_IWUSR
#define   KEY_FILE_NAME 	"/sbin/brt-server"

typedef struct {
	char ip_addr[MAX_TXT_BUFFER_LEN];
	char net_mask[MAX_TXT_BUFFER_LEN];
	char gw[MAX_TXT_BUFFER_LEN];
	char dev_name[MAX_DEV_NAME_LEN];
	char status[MAX_DEV_NAME_LEN];
}BIND_ROUTE_MSG_INFO_BODY;

typedef struct {
	char orgDevName[32];
	char newDevName[32];
}DEV_MAP_NODE;

typedef struct {
	char counts[MAX_TXT_BUFFER_LEN];
	DEV_MAP_NODE net_dev_name_map_list[3];//max 3 vlan .
	char status[MAX_DEV_NAME_LEN];
}NET_DEV_NAME_MAP_MSG_INFO_BODY;

struct msgtype { 
	long mtype;
	char buf[512];
};

enum MSG_TYPE{
	TYPE_BIND_ROUTE=1,
	TYPE_DEV_NAME_MAP=2,
	TYPE_DEV_INFO=3,
};
#endif

