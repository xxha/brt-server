#ifndef __ARP__H
#define __ARP__H
int parse_arp(unsigned char *buf,int len,int vlan_len,unsigned int ip_addr,unsigned char *dev_name,unsigned char *result);
#endif
