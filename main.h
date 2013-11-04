#ifndef __MAIN_H
#define __MAIN_H

#define MAX_DEV_NAMES 3
#define MAX_FRAME_SIZE 1518
#define ETHER_PROTO_OFF 12

#define PROC_FILE_NAME_HEAD "/proc/sys/net/ipv4/conf/"
#define PROC_FILE_NAME_TAIL "/arp_ignore"
#define SYS_CTL_HEAD "echo 1 > /proc/sys/net/ipv4/conf/"
#define SYS_CTL_TAIL "/arp_ignore "

#define __DEBUG__  
#ifdef __DEBUG__  
#define DEBUG(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)  
#else  
#define DEBUG(format,...)  
#endif  

#define ETH_IP /*enable protocol parse */
#define MANAGE_PORT "eth1"
#define ETH_ARP
typedef enum {
	FAIL_TO_OPEN_ROUTE_FILE = 0,
	FAIL_ILLEGAL_PARA_FOR_THREAD,
	FAIL_TO_DEL_UNIQ_QUE_FILE,
	FAIL_TO_CREATE_UNIQ_QUE_FILE,
} BRT_SERVER_EXIT_VALUE;


enum ERR_VALUE {
	ERR_CREATE_SOCKET,
	ERR_GET_DEV_INDEX,
	ERR_GET_DEV_IP,
	ERR_BIND_DEVICE,
	ERR_INIT_THREAD_ATTR,
	ERR_CREATE_THREAD,
	ERR_CREATE_THREAD_IPC,
	ERR_NO_PARA,
	ERR_ALLOC_MEM,
	ERR_RCV_SOCK_DATA,
};

typedef enum {
	CMD_RUN,
	CMD_EXIT,
} CMD_INFO;

typedef enum {
	STATUS_RUN,
	STATUS_IDEL,
	STATUS_DEAD,
}STATUS_INFO;

typedef struct {
#define MAX_ETH_NAME_LEN 32
#define MAX_CMD_NAME_LEN 128
	int sock;
	char name[MAX_ETH_NAME_LEN];
	char alias[MAX_ETH_NAME_LEN];	
	unsigned int ip_addr;
	char ip_deci_dot[32];
	unsigned char mac[6];
	char gateway[32];
	char netmask[32];
	int index;
	volatile int update;
	volatile CMD_INFO cmd;
	volatile STATUS_INFO status;
	char work;			/* 0 = sleep, 1 = work. */
	void *pcap;
	int order;
}SOCKET_INFO;

typedef struct {
	unsigned char hiByte;
	unsigned char loByte;
	int (*parse)(unsigned char *buf,int len,int vlan_len,unsigned int ip_addr,unsigned char *dev_name,unsigned char *result);
}PROTO_OPS;

typedef struct {
	volatile CMD_INFO cmd;
	volatile STATUS_INFO status;
}IPC_MONITOR_INFO;

int create_pcap(SOCKET_INFO *sockPtr);
#endif
