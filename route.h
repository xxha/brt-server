#ifndef __ROUTE__H
#define __ROUTE__H

#define EMPTY_GW "0.0.0.0"
int keep_route(char *dst_addr, char *net_mask, char *gw, char *dev_name, char *host_ip);
int open_route_file();
void close_route_file();
#endif
