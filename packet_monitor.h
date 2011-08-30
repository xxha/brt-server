#ifndef __PACKET_MONITOR_H
#define __PACKET_MONITOR_H
typedef struct {
	unsigned char hiByte;
	unsigned char loByte;
}PROTO_TYPE_II;
void *packet_monitor(void *para);
int get_dev_mac_address(SOCKET_INFO *sockInfo);
int bind_socket_to_devname(SOCKET_INFO *sockInfo);
#endif
