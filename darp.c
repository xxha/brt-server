#include <pcap.h> 
#include <stdlib.h> 
#include <string.h> 
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <pthread.h>
#include <unistd.h>
#include "main.h"


#define	IP_SIZE			32
#define DEV_NAME_LEN    	64
#define SIZE_ETHERNET 		14
#define NETDEV_COUNTS 		3
#define ETHER_ADDR_LEN  	6
#define	CMD_BUFFER_SIZE		128
#define MAXBYTES2CAPTURE 	2048 

#define IP_HL(ip)	       (((ip)->ip_vhl) & 0x0f)

unsigned int g_netadd[NETDEV_COUNTS];

int gethostdev(char * hostip, char * dev);

/* Ethernet header */
struct sniff_ethernet {
	u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
	u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
	u_short ether_type;			/* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
	u_char  ip_vhl;				/* version << 4 | header length >> 2 */
	u_char  ip_tos;				/* type of service */
	u_short ip_len;				/* total length */
	u_short ip_id;				/* identification */
	u_short ip_off;				/* fragment offset field */
	#define IP_RF 0x8000			/* reserved fragment flag */
	#define IP_DF 0x4000			/* dont fragment flag */
	#define IP_MF 0x2000			/* more fragments flag */
	#define IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_char  ip_ttl;				/* time to live */
	u_char  ip_p;				/* protocol */
	u_short ip_sum;				/* checksum */
	struct  in_addr ip_src,ip_dst;		/* source and dest address */
};

int arpcheck(char * hostip, char * interface)
{
	char devname[DEV_NAME_LEN];

	if(gethostdev(hostip, devname) == 0) {
		DEBUG("devname = %s, interface = %s\n", devname, interface);
		if(strcmp(devname, interface) == 0) {
			return 0;
		}
	}

	return -1;
}

int gethostdev(char * hostip, char * dev)
{
	int r;
	int flgs, ref, use, metric, mtu, win, ir;
	unsigned long d, g, m;
	char * ip;
	char ipaddr[IP_SIZE];
	char devname[DEV_NAME_LEN];
	struct in_addr addr;
	FILE *fp;

	fp = fopen("/proc/net/route", "r");
	if (fscanf(fp, "%*[^\n]\n") < 0) { /* Skip the first line. */
		printf("Read routing table error\n");
		fclose(fp);
		return -1;
	}

	while (1){
		r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
				devname, &d, &g, &flgs, &ref, &use,
				&metric, &m, &mtu, &win, &ir);
		if (r != 11) {
			if ((r < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
				break;
			}
			printf("fscanf failed\n");
			fclose(fp);
			return -1;
		}

		if (!(flgs & RTF_UP)) { /* Skip interfaces that are down. */
			printf("Ignore %s, which is down\n", devname);
			continue;
		}

		addr.s_addr = d;
		ip = inet_ntoa(addr);
		strcpy(ipaddr, ip);

		DEBUG("devname = %s, hostip = %s, ipaddr = %s\n", devname, hostip, ipaddr);
		if(strcmp(hostip, ipaddr) == 0){
			DEBUG("The interface name for IP: %s is %s\n", ipaddr, devname);
			strcpy(dev, devname);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	return -1;
}

int findip(char *dev, char *ipaddr,unsigned int *ip32,unsigned int *netmask32)
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	pcap_addr_t *a;
	int status;
	char errbuf[PCAP_ERRBUF_SIZE];

	if(dev == NULL)
		return -1;

	status = pcap_findalldevs(&alldevs, errbuf);
	if(status != 0) {
		printf("%s\n", errbuf);
		return 1;
	}

	for(d=alldevs; d!=NULL; d=d->next) {
		if(strcmp(d->name, dev) == 0) {
			for(a=d->addresses; a!=NULL; a=a->next) {
				if(a->addr->sa_family == AF_INET) {
					sprintf(ipaddr, "%s", inet_ntoa(((struct sockaddr_in*)a->addr)->sin_addr));
					if(ip32)
						*ip32 = ((struct sockaddr_in*)a->addr)->sin_addr.s_addr;
					if(netmask32)
						*netmask32 = ((struct sockaddr_in*)a->netmask)->sin_addr.s_addr;
				}
			}
			break;
		}
	}

	pcap_freealldevs(alldevs);

	return 0;
}

void *darp_process(void *para)
{
	int ret = 0;
	int iplen = 0;
	int size_ip;
	unsigned int src_netaddress32;
	unsigned int local_ip32;
	unsigned int src_ip32;
	unsigned int local_netmask32;
	unsigned int pnetaddress32;
	char errbuf[PCAP_ERRBUF_SIZE];    		/* Error buffer */ 
	char ipaddr[IP_SIZE];
	char dipaddr[IP_SIZE];
	char sipaddr[IP_SIZE];
	char *myip;
	char filter_exp[] = "icmp"; 
	char *p_interface = NULL;
	char local_gw[32];
	char cmd[CMD_BUFFER_SIZE];
	char dethaddr[CMD_BUFFER_SIZE];
	unsigned char *pnet = NULL;
	const unsigned char *packet = NULL;		/* Received raw data */ 
	struct bpf_program filter;			/* Place to store the BPF filter program  */ 
	struct pcap_pkthdr pkthdr;			/* Packet information (timestamp,size...) */ 
	const struct sniff_ethernet *ethernet;  	/* The ethernet header [1] */
	const struct sniff_ip *ip;
	bpf_u_int32 netaddr = 0, mask = 0;    		/* To Store network address and netmask */ 
	SOCKET_INFO *sockPtr;
	pcap_t *descr = NULL;	     		/* Network interface handler */ 
	pcap_t *new_descr = NULL;       
	
	if(para == NULL)
	{
		printf("ERROR: Must set net interface to run darp process\n");
		ret = -1;
		pthread_exit(&ret);
	}

	sockPtr = (SOCKET_INFO *)para;
	p_interface = (char *)(&(sockPtr->name));
	printf("darp process run on net interface %s\n", p_interface);

	strcpy(local_gw, sockPtr->gateway);
	memset(errbuf, 0, PCAP_ERRBUF_SIZE); 
	memset(ipaddr, 0, IP_SIZE);
	memset(dipaddr, 0, IP_SIZE);
	memset(sipaddr, 0, IP_SIZE);

	/* Open network device for packet capture */ 
	while(1) {
		if ((descr = pcap_open_live(p_interface, MAXBYTES2CAPTURE, 0,  512, errbuf)) == NULL) {
			DEBUG("pcap oepn failed: %s\n", errbuf);
			sleep(10);
			continue;
		} else {
			printf("%s: pcap open dev successfully\n", p_interface);
			break;
		}
	}

	/* Look up info from the capture device. */ 
	while(1) {
		if (pcap_lookupnet( p_interface , &netaddr, &mask, errbuf) == -1) {
			//DEBUG("pcap lookupnet failed: %s\n", errbuf);
			sleep(10);
			continue;
		} else {
			printf("%s: pcap lookupnet successfully\n", p_interface);
			break;
		}	
	}

	/* Get local ip and netmask for specific net interface */
	while(1) {
		if(findip(p_interface, ipaddr, &local_ip32, &local_netmask32) != 0) {
			DEBUG("get IP address for dev: %s failed\n", p_interface);
			sleep(10);
			continue;
		} else {
			printf("%s: pcap get IP address successfully\n", p_interface);
			break;
		}	
	}

	DEBUG("the net device order is %d\n", sockPtr->order );
	if(sockPtr->order < 3 && sockPtr->order >= 0) {
		g_netadd[sockPtr->order] = local_ip32 & local_netmask32;
		pnet = (unsigned char *)(&g_netadd[sockPtr->order]);
		printf("%s: get netaddress is %d.%d.%d.%d\n", p_interface, *pnet, *(pnet+1), *(pnet+2), *(pnet+3));
	} else
		printf("%s: error, socket order %d is invalid.\n",p_interface, sockPtr->order);
	
	iplen = strlen(ipaddr);
	DEBUG("%s: IP address: %s, len: %d\n", p_interface, ipaddr, iplen);

	/* Compiles the filter expression into a BPF filter program */ 
	while(1) {
		if (pcap_compile(descr, &filter, filter_exp, 1, mask) == -1) {
			printf("pcap compile icmp failed: %s\n", pcap_geterr(descr) );
			sleep(10);
			continue;
		} else {
			printf("%s: pcap compile icmp successfully\n", p_interface);
			break;
		}

		if (pcap_compile(descr, &filter, "arp", 1, mask) == -1) {
			printf("compile arp failed: %s\n", pcap_geterr(descr) );
			sleep(10);
			continue;
		} else {
			printf("%s: pcap compile arp successfully\n", p_interface);
			break;
		}
	}

	/* Load the filter program into the packet capture device. */ 
	while(1) {
		if (pcap_setfilter(descr, &filter) == -1) {
			DEBUG("pcap setfilter failed: %s\n", pcap_geterr(descr) );
			sleep(10);
			continue;
		} else {
			printf("%s: pcap compile successfully\n", p_interface);
			break;
		}
	}
	
	sockPtr->pcap = (void *)descr;
	sockPtr->status = STATUS_RUN;
	sockPtr->work = 1;
	
	while(1) {
		if(!sockPtr->work) {
			sleep(2);
			continue;
		}

		/* Check the exit command. */
		if(sockPtr->cmd == CMD_EXIT) {
			sockPtr->status = STATUS_DEAD;
			ret = -1;
			pthread_exit(&ret);
		}
		
		new_descr = (pcap_t *)(sockPtr->pcap);
		
		if ((packet = pcap_next(new_descr, &pkthdr)) == NULL) {  /* Get one packet */ 
			fprintf(stderr, "ERROR: Error getting the packet, %s\n", errbuf);
			continue;
		}

		/* parse ip header offset */
		ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
		size_ip = IP_HL(ip) * 4;
		if (size_ip < 20) {
			printf("Invalid IP header length: %u bytes\n", size_ip);
			continue;
		}

		/* get source ip addr */
		myip = inet_ntoa(ip->ip_src);
		strcpy(sipaddr, myip);

		/* get dest ip addr */
		myip = inet_ntoa(ip->ip_dst);
		strcpy(dipaddr, myip);

		/* unsigned int source ip */
		src_ip32 = ip->ip_src.s_addr;

		memset(ipaddr, 0, IP_SIZE);
		if(findip(p_interface, ipaddr, &local_ip32, &local_netmask32) != 0) {
			printf("ERROR: Can't get IP address for dev: %s\n", p_interface);
			continue;
		}

		DEBUG("My IP: %s, Target IP: %s, Source IP: %s\n", ipaddr, dipaddr, sipaddr);

		/* start to process the net address */
		pnetaddress32 = local_ip32 & local_netmask32;
		src_netaddress32 = src_ip32 & local_netmask32;

		if(sockPtr->order < 3 && sockPtr->order >= 0)
			g_netadd[sockPtr->order] = pnetaddress32;
		else
			printf("ERROR:%s the order is invalid %d\n", p_interface, sockPtr->order);

		DEBUG("xxha: ------------- ipaddr = %s\n", ipaddr);
		DEBUG("xxha: ------------- dipaddr = %s\n", dipaddr);
		DEBUG("xxha: ------------- iplen = %d\n", iplen);

		/* arp process */
		if(memcmp(ipaddr, dipaddr, iplen) == 0) {
			if(arpcheck(sipaddr, p_interface) == 0) {
				ethernet = (struct sniff_ethernet*)(packet);

				memset(dethaddr, 0, sizeof(dethaddr));
				sprintf(dethaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
					ethernet->ether_shost[0],ethernet->ether_shost[1],
					ethernet->ether_shost[2], ethernet->ether_shost[3],
					ethernet->ether_shost[4], ethernet->ether_shost[5]);

				memset(cmd, '\0', CMD_BUFFER_SIZE);
				sprintf(cmd, "arp -s %s %s -i %s\n", sipaddr, dethaddr, p_interface);
				system(cmd);

				DEBUG("Add IP: %s, MAC: %s, if: %s to ARP table\n",
							sipaddr, dethaddr, p_interface);
				DEBUG("ARP command: %s\n", cmd);

			} else {
				DEBUG("We don't need add %s to the ARP table\n", sipaddr);
			}
		}
	}
	return 0;
}

int create_pcap(SOCKET_INFO *sockPtr)
{
	int iplen = 0;
	char errbuf[PCAP_ERRBUF_SIZE];    /* Error buffer			   */ 
	char ipaddr[IP_SIZE];
	char filter_exp[] = "icmp"; 
	char *p_interface = NULL;
	struct bpf_program filter;	/* Place to store the BPF filter program  */ 
	pcap_t *descr = NULL;  
	bpf_u_int32 netaddr=0, mask=0;    /* To Store network address and netmask   */ 

	p_interface = (char *)&(sockPtr->alias);
	printf("change the new pcap fd to new interface %s\n",p_interface);

	/* Open network device for packet capture */ 
	if ((descr = pcap_open_live(p_interface, MAXBYTES2CAPTURE, 0,  512, errbuf))==NULL) {
		fprintf(stderr, "ERROR open fail: %s\n", errbuf);
		return -1;
	} else {
		printf("pcap open dev successfully\n");
	}

	/* Look up info from the capture device. */ 
	if (pcap_lookupnet( p_interface , &netaddr, &mask, errbuf) == -1) {
		fprintf(stderr, "ERROR lookupnet fail: %s\n", errbuf);
		return -1;
	}

	if( findip( p_interface, ipaddr,NULL,NULL) != 0) {
		fprintf(stderr, "ERROR: Can't get IP address for dev: %s\n", p_interface);
		return  -1;
	}

	iplen = strlen(ipaddr);
	printf("%s IP address: %s, len: %d\n", p_interface, ipaddr, iplen);

	/* Compiles the filter expression into a BPF filter program */ 
	if (pcap_compile(descr, &filter, filter_exp, 1, mask) == -1) {
		fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
		return -1;
	}

	/* Load the filter program into the packet capture device. */ 
	if (pcap_setfilter(descr, &filter) == -1) {
		fprintf(stderr, "ERROR setfilter fail: %s\n", pcap_geterr(descr) );
		return -1;
	}

	/* Compiles the filter expression into a BPF filter program */
	if (pcap_compile(descr, &filter, "arp", 1, mask) == -1) {
		fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
		return -1;
	}

	/* Load the filter program into the packet capture device. */
	if (pcap_setfilter(descr, &filter) == -1) {
		fprintf(stderr, "ERROR setfilter fail: %s\n", pcap_geterr(descr) );
		return -1;
	}


	/* firstly close the old pcap fd */
	if(sockPtr->pcap)
		pcap_close(sockPtr->pcap);
	
	sockPtr->pcap = (void *)descr;

	return 0;
}
/* EOF */
