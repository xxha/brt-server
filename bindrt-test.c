#include <stdio.h>
#include <string.h>
#include "bindrt.h"
#include "msgque.h"

void usage(unsigned char *process){
	printf("Usage:\t%s -r dst_ip net_mask gate_way dev_name\n",process);
	printf("\t%s -v org_dev_name new_dev_name\n",process);
	printf("\t%s -n dev_name ip netmask gateway",process);
	printf("For exmaples\n:\t%s -r 192.168.8.110 255.255.255.0 192.168.8.1 eth0 \n",process);
	printf("\t%s -v eth0.1 eth0.99 \n",process);
	printf("\t%s -n eth0 192.168.8.234 255.255.255.0 192.168.8.1 \n",process);
	
}
int bind_route(unsigned char *argv[]){
	BIND_RET_INFO bindRTINFO;

	unsigned char dst_ip[16],net_mask[16],gw[16],dev_name[16];

	strcpy(dst_ip,argv[0]);
	strcpy(net_mask,argv[1]);
	strcpy(gw,argv[2]);
	strcpy(dev_name,argv[3]);
		        	printf("%s:%d\n",__FILE__,__LINE__);

	bindRTINFO=v100p_bind_route(dst_ip,net_mask,gw,dev_name);
		        	printf("%s:%d\n",__FILE__,__LINE__);

	printf("%s\n",bindRTINFO.errInfo);
	return 0;
}
int set_vlan_device(unsigned char *argv[]){
	NET_DEV_NAME_RET_INFO netDevMapInfo;
	NET_DEV_NAME_MAP_NODE netDevNameMapList[10];
	strcpy(netDevNameMapList[0].orgDevName,argv[0]);
	strcpy(netDevNameMapList[0].newDevName,argv[1]);
	
	netDevMapInfo=v100p_net_name_map(netDevNameMapList,1);
	printf("%s\n",netDevMapInfo.errInfo);
}
int set_gateway(unsigned char *argv[]){

	DEV_NET_INFO devnetinfo;
	NET_DEV_INFO_RET_INFO netDevNameRetInfo;
	
	strcpy(devnetinfo.dev_name,argv[0]);
	strcpy(devnetinfo.ip,argv[1]);
	strcpy(devnetinfo.netmask,argv[2]);
	strcpy(devnetinfo.gateway,argv[3]);
	devnetinfo.mac[0]=0x00;
	devnetinfo.mac[1]=0x18;
	devnetinfo.mac[2]=0x63;
	devnetinfo.mac[3]=0x00;
	devnetinfo.mac[4]=0x00;
	devnetinfo.mac[5]=0x10;
	devnetinfo.work=1;
	netDevNameRetInfo=v100p_set_net_info(&devnetinfo);
	printf("ret value:%d ret info:%s\n",netDevNameRetInfo.val,netDevNameRetInfo.errInfo);
}
int main(int argc, char *argv[]){

	int i;
	
	if(argc<2){
		usage(argv[0]);
		return -1;
	}
		
	if(strcmp(argv[1],"-r")==0){
		//return bind_route((unsigned char *)&argv[2]);
		return bind_route((unsigned char *)&argv[2]);
		
	}
	if(strcmp(argv[1],"-v")==0){
		return set_vlan_device((unsigned char *)&argv[2]);
	}
	if(strcmp(argv[1],"-n")==0){
		return set_gateway((unsigned char *)&argv[2]);
	}else{
		usage(argv[0]);
		return -1;
	}

	return 0;
}
