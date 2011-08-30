#ifndef __ROUTE__H
#define __ROUTE__H

#define EMPTY_GW "0.0.0.0"
int keep_route(unsigned char *dst_addr,unsigned char *net_mask,unsigned char *gw,unsigned char *dev_name,unsigned char *host_ip);
int open_route_file();
void close_route_file();
#endif
