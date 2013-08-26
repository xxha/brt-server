#ifndef __PUB__H
#define __PUB__H
//#define DEBUG 
#define ETH_IP /*enable protocol parse */
#define MANAGE_PORT "eth1"
#define ETH_ARP
typedef enum{
	FAIL_TO_OPEN_ROUTE_FILE=0,
	FAIL_ILLEGAL_PARA_FOR_THREAD,
	FAIL_TO_DEL_UNIQ_QUE_FILE,
	FAIL_TO_CREATE_UNIQ_QUE_FILE,

}BRT_SERVER_EXIT_VALUE;
void print_buf(unsigned char *buf,int size,int col);
#endif
