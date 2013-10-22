#ifndef __MSQ__QUE__H
#define __MSG__QUE__H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define   PERM S_IRUSR|S_IWUSR
#define   BUFFER 255
#define KEY_FILE_NAME "/sbin/brt-server"
#define MAX_TXT_BUFFER_LEN 64
#define MAX_DEV_NAME_LEN 8

typedef struct{
	unsigned char ip_addr[MAX_TXT_BUFFER_LEN];
    unsigned char net_mask[MAX_TXT_BUFFER_LEN];
    unsigned char gw[MAX_TXT_BUFFER_LEN];
    unsigned char dev_name[MAX_DEV_NAME_LEN];
    unsigned char status[MAX_DEV_NAME_LEN];
}BIND_ROUTE_MSG_INFO_BODY;

typedef struct{
	unsigned char orgDevName[32];
	unsigned char newDevName[32];
}DEV_MAP_NODE;

typedef struct {
	unsigned char counts[MAX_TXT_BUFFER_LEN];
	DEV_MAP_NODE net_dev_name_map_list[3];//max 3 vlan .
	unsigned char status[MAX_DEV_NAME_LEN];
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

