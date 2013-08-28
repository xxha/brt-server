#include <pcap.h> 
#include <stdlib.h> 
#include <string.h> 

#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include "main.h"


#define MAXBYTES2CAPTURE 2048 

#define	IP_SIZE		32
#define	CMD_BUFFER_SIZE	128

#define DEV_NAME_LEN    64

#define SIZE_ETHERNET 14
#define ETHER_ADDR_LEN  6

#define NETDEV_COUNTS 3
unsigned int g_netadd[NETDEV_COUNTS];

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

//#define	DEBUG	1

int routecheck(char * hostip, char * interface)
{
        char devname[DEV_NAME_LEN];
        char cmd[CMD_BUFFER_SIZE];

        if(gethostdev(hostip, devname) == 0){
                if(strcmp(devname, interface) == 0){
                        return -1;
                }else{
                        printf("devname: %s, interface: %s\n", devname, interface);
                        sprintf(cmd, "route del %s\n", hostip);
                        system(cmd);
                        return 0;
                }
        }else{
                return 0;
        }
}

int gethostdev(char * hostip, char * dev)
{
        char devname[DEV_NAME_LEN], flags[16], *sdest, *sgw;
        int flgs, ref, use, metric, mtu, win, ir;
        unsigned long d, g, m;
        struct sockaddr_in s_addr;
        struct in_addr mask;
        struct in_addr addr;
        char ipaddr[IP_SIZE];
        char * ip;


        FILE *fp;
        int r;

        fp = fopen("/proc/net/route", "r");

        if (fscanf(fp, "%*[^\n]\n") < 0) { /* Skip the first line. */
                printf("Read routing table error\n");
                fclose(fp);
                return -1;
        }

        while (1){
                r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                                   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                                   &mtu, &win, &ir);
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

#ifdef DEBUG
                printf("%s : %s\n", devname, ipaddr);
#endif
                if(strcmp(hostip, ipaddr) == 0){
#ifdef DEBUG
                        printf("The interface name for IP: %s is %s\n", ipaddr, devname);
#endif
                        strcpy(dev, devname);
                        fclose(fp);
                        return 0;
                }
        }

        fclose(fp);

        return -1;
}

int findip(char * dev, char * ipaddr,unsigned int *ip32,unsigned *netmask32)
{
        pcap_if_t *alldevs;
        pcap_if_t *d;
        pcap_addr_t *a;
	int status;
	char errbuf[PCAP_ERRBUF_SIZE];

	if(dev == NULL) return;

	status = pcap_findalldevs(&alldevs, errbuf);
        if(status != 0) {
                printf("%s\n", errbuf);
                return 1;
        }

        for(d=alldevs; d!=NULL; d=d->next) {
		if( strcmp( d->name, dev ) == 0 ){
            for(a=d->addresses; a!=NULL; a=a->next) {
                if(a->addr->sa_family == AF_INET)
                {
		         sprintf(ipaddr, " %s", inet_ntoa(((struct sockaddr_in*)a->addr)->sin_addr));
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

//int main(int argc, char *argv[])
void *droute_process(void *para)
{
	int i=0; 
	bpf_u_int32 netaddr=0, mask=0;    /* To Store network address and netmask   */ 
	struct bpf_program filter;        /* Place to store the BPF filter program  */ 
	char errbuf[PCAP_ERRBUF_SIZE];    /* Error buffer                           */ 
	pcap_t *descr = NULL;             /* Network interface handler              */ 
	struct pcap_pkthdr pkthdr;        /* Packet information (timestamp,size...) */ 
	const unsigned char *packet=NULL; /* Received raw data                      */ 
	char filter_exp[] = "icmp"; 
    
	struct in_addr addr;
	char ipaddr[IP_SIZE];
	char dipaddr[IP_SIZE];
	char sipaddr[IP_SIZE];
	char * myip;
	int iplen = 0;
	char cmd[CMD_BUFFER_SIZE];
        char dethaddr[CMD_BUFFER_SIZE];
        const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */

	const struct sniff_ip *ip;
	int size_ip;
	unsigned int local_ip32;
	unsigned int src_ip32;
	unsigned int local_netmask32;
	unsigned int pnetaddress32;
	unsigned char *pnet = NULL;
	char *p_interface = NULL;
	SOCKET_INFO *sockPtr;
	char local_gw[32];
	pcap_t *new_descr = NULL;       
	int ret=0;
	int netadd_issame_flag = 0;
	unsigned int src_netaddress32;
	
	if(para == NULL)
	{
		printf("ERROR:MUST set net interface to run droute process\n");
		ret=-1;
		pthread_exit(&ret);
	}

	sockPtr=(SOCKET_INFO *)para;
	p_interface = (char *)(&(sockPtr->name));
	printf("droute process run on net interface %s\n",p_interface);

	strcpy(local_gw,sockPtr->gateway);
	
	memset(errbuf, 0, PCAP_ERRBUF_SIZE); 

	memset(ipaddr, 0, IP_SIZE);
	memset(dipaddr, 0, IP_SIZE);
	memset(sipaddr, 0, IP_SIZE);

	//if (argc != 2){ 
	//	printf("USAGE: droute <interface>\n"); 
	//	exit(1); 
	//}
 
	 /* Open network device for packet capture */ 
	while(1)
	{
		if ((descr = pcap_open_live(p_interface, MAXBYTES2CAPTURE, 0,  512, errbuf))==NULL){
#ifdef DEBUG
			fprintf(stderr, "ERROR open fail: %s\n", errbuf);
			printf("process wait and poll for net device load successfully\n");
#endif
			sleep(10);
			continue;
		}
		else{
			printf("pcap open dev successfully\n");
			break;
		}
	}
	/* Look up info from the capture device. */ 
	while(1)
	{
		if (pcap_lookupnet( p_interface , &netaddr, &mask, errbuf) == -1){
#ifdef DEBUG
			fprintf(stderr, "ERROR lookupnet fail: %s\n", errbuf);
#endif
			sleep(10);
			continue;
		}else{
			printf("pcap lookupnet successfully\n");
			break;
		}	
	}

	while(1)
	{
		//if( findip( p_interface, ipaddr,NULL,NULL) != 0){
		
		if( findip( p_interface, ipaddr,&local_ip32,&local_netmask32) != 0){
#ifdef DEBUG
			fprintf(stderr, "ERROR: Can't get IP address for dev: %s\n", p_interface);
#endif
			sleep(10);
			continue;
		}else{
			printf("pcap get IP address successfully\n");
			break;
		}	
	}

	printf("the net device order is %d\n",sockPtr->order );
	if(sockPtr->order < 3&&sockPtr->order>=0)
	{
		g_netadd[sockPtr->order] = local_ip32&local_netmask32;
		pnet = (char*)(&g_netadd[sockPtr->order]);
		printf("get %s netaddress is %d.%d.%d.%d\n",p_interface,*pnet,*(pnet+1),*(pnet+2),*(pnet+3));
	}else
	  fprintf(stderr,"ERROR:%s the order is invalid %d\n",p_interface,sockPtr->order);
	
	
	iplen = strlen(ipaddr);

	printf("%s IP address: %s, len: %d\n", p_interface, ipaddr, iplen);
	while(1)
	{
		/* Compiles the filter expression into a BPF filter program */ 
		if (pcap_compile(descr, &filter, filter_exp, 1, mask) == -1){
#ifdef DEBUG
			fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
#endif

			sleep(10);
			continue;
			//ret=-1;
			//pthread_exit(&ret);
		}else{
			printf("pcap compile successfully\n");
			break;
		}

                if (pcap_compile(descr, &filter, "arp", 1, mask) == -1){
#ifdef DEBUG
                        fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
#endif

                        sleep(10);
                        continue;
                        //ret=-1;
                        //pthread_exit(&ret);
                }else{
                        printf("pcap compile successfully\n");
                        break;
                }


	}
	/* Load the filter program into the packet capture device. */ 

	while(1)
	{
		if (pcap_setfilter(descr, &filter) == -1){
#ifdef DEBUG
			fprintf(stderr, "ERROR setfilter fail: %s\n", pcap_geterr(descr) );
#endif
			sleep(10);
			continue;
			//ret=-1;
			//pthread_exit(&ret);
		}else{
			printf("pcap compile successfully\n");
			break;
		}	
	}
	
	sockPtr->pcap = (void *)descr;
	sockPtr->status=STATUS_RUN;
	sockPtr->work = 1;
	
	while(1){ 
		
		if(!sockPtr->work){
			printf("droute process work is 0\n");
			sleep(2);
			continue;
		}

		//-- Check the exit command.
		if(sockPtr->cmd==CMD_EXIT){
			sockPtr->status=STATUS_DEAD;
			ret=-1;
			pthread_exit(&ret);
		}
		
		new_descr = (pcap_t *)(sockPtr->pcap);
		
		if ( (packet = pcap_next(new_descr,&pkthdr)) == NULL){  /* Get one packet */ 
			fprintf(stderr, "ERROR: Error getting the packet.\n", errbuf);
			continue;
		}

		/* define/compute ip header offset */
		ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
		size_ip = IP_HL(ip)*4;
		if (size_ip < 20) {
			printf("   * Invalid IP header length: %u bytes\n", size_ip);
			continue;
		}

		myip = inet_ntoa(ip->ip_src);
		strcpy(sipaddr, myip);

		myip = inet_ntoa(ip->ip_dst);
		strcpy(dipaddr, myip);
        //add for netmask calculate
		src_ip32 = ip->ip_src.s_addr;

		memset(ipaddr, 0, IP_SIZE);
		if( findip( p_interface, ipaddr,&local_ip32,&local_netmask32) != 0){
			fprintf(stderr, "ERROR: Can't get IP address for dev: %s\n", p_interface);
			continue;
		}

		
		//memset(ipaddr, 0, IP_SIZE);
		//    if( findip( argv[1], ipaddr) != 0){
		//           fprintf(stderr, "ERROR: Can't get IP address for dev: %s\n", argv[1]);
		//           continue;
		// }

#ifdef DEBUG
		printf("My IP: %s, Target IP: %s, Source IP: %s\n", ipaddr, dipaddr, sipaddr);
#endif

        //start to process the net address ,compare net address of all net device ;
		pnetaddress32 = local_ip32&local_netmask32;
		src_netaddress32= src_ip32&local_netmask32;
        
		netadd_issame_flag = 0;
		
		for(i=0;i<NETDEV_COUNTS;i++)
		{
            //it's my self;
//			if(i == sockPtr->order)
//				continue;
			if(g_netadd[i]>0)
			{
				if(g_netadd[i] == pnetaddress32)
				{
				    printf("find %d and %d have the same net address\n",sockPtr->order,i);
					netadd_issame_flag =1;
					break;
				}
				
			}else
			  printf("%d net interface don't assign the net address\n",i);
		}

		if(sockPtr->order < 3&&sockPtr->order>=0)
			g_netadd[sockPtr->order]= pnetaddress32;
		else
			fprintf(stderr,"ERROR:%s the order is invalid %d\n",p_interface,sockPtr->order);

		if(netadd_issame_flag == 0)
		{
		     //#ifdef DEBUG
			pnet = (char*)(&src_netaddress32);
			printf("local net%u src net%u\n",pnetaddress32,src_netaddress32);
			printf("src net address:%d.%d.%d.%d\n",*pnet,*(pnet+1),*(pnet+2),*(pnet+3));
			 //#endif
		    
			if((local_ip32&local_netmask32) == (src_ip32&local_netmask32))
			{
			    //#ifdef DEBUG
			    pnet = (char*)(&pnetaddress32);
			    printf("This interface and srcip of ping in same net address:%d.%d.%d.%d\n",*pnet,*(pnet+1),*(pnet+2),*(pnet+3));
			    //#endif
				continue;
			}
		}
		else
		    	printf("Two net devices have same net address, so need to add to route table\n");
		
		//net address process completely;
		
		if( memcmp(ipaddr+1, dipaddr, iplen-1) == 0 ){
		        if(routecheck(sipaddr, p_interface) == 0){
                                ethernet = (struct sniff_ethernet*)(packet);
                                memset(dethaddr, 0, sizeof(dethaddr));
                                sprintf(dethaddr, "%02x:%02x:%02x:%02x:%02x:%02x", ethernet->ether_shost[0], ethernet->ether_shost[1], ethernet->ether_shost[2], ethernet->ether_shost[3], ethernet->ether_shost[4], ethernet->ether_shost[5]);
#ifdef DEBUG
                                printf("Add IP: %s to %s, MAC: %s to ARP table\n", sipaddr, p_interface, dethaddr);
#endif
                                if(strcmp(local_gw,"0.0.0.0"))
                                        sprintf(cmd, "route add %s gw %s dev %s\n", sipaddr, local_gw, p_interface);
                                else
                                        sprintf(cmd, "route add %s dev %s\n", sipaddr, p_interface);

                                system(cmd);

                                memset(cmd, '\0', CMD_BUFFER_SIZE);

#ifdef DEBUG
                                printf("cmd: %s, sipaddr: %s, dethaddr: %s\n", cmd, sipaddr, dethaddr);
#endif

                                sprintf(cmd, "arp -s %s %s\n", sipaddr, dethaddr);
#ifdef DEBUG
                                printf("ARP command: %s\n", cmd);
#endif
                                system(cmd);

#if 0
#ifdef DEBUG
		                printf("Add IP: %s to %s\n", sipaddr, p_interface);
#endif
				if(strcmp(local_gw,"0.0.0.0"))
					sprintf(cmd, "route add %s gw %s dev %s\n", sipaddr,local_gw, p_interface);
				else
					sprintf(cmd, "route add %s dev %s\n", sipaddr, p_interface);
				
                printf("add route:%s\n",cmd);
				system(cmd);
#endif


		        }else{
#ifdef DEBUG
		                printf("We don't need add %s to the routing table\n", sipaddr);
#endif
		        }
		}
	} 
	return 0; 
}


int create_pcap(SOCKET_INFO *sockPtr)
{
	char *p_interface = NULL;
	bpf_u_int32 netaddr=0, mask=0;    /* To Store network address and netmask   */ 
	struct bpf_program filter;        /* Place to store the BPF filter program  */ 
	char errbuf[PCAP_ERRBUF_SIZE];    /* Error buffer                           */ 
	pcap_t *descr = NULL;  
	char ipaddr[IP_SIZE];
	int iplen = 0;
	char filter_exp[] = "icmp"; 


	p_interface = &(sockPtr->alias);
	printf("change the new pcap fd to new interface %s\n",p_interface);
	/* Open network device for packet capture */ 
	if ((descr = pcap_open_live(p_interface, MAXBYTES2CAPTURE, 0,  512, errbuf))==NULL){
		fprintf(stderr, "ERROR open fail: %s\n", errbuf);
		return -1;
	}
	else{
		printf("pcap open dev successfully\n");
	}

	/* Look up info from the capture device. */ 
	if (pcap_lookupnet( p_interface , &netaddr, &mask, errbuf) == -1){
		fprintf(stderr, "ERROR lookupnet fail: %s\n", errbuf);
		return -1;
	}

	if( findip( p_interface, ipaddr,NULL,NULL) != 0){
		fprintf(stderr, "ERROR: Can't get IP address for dev: %s\n", p_interface);
		return  -1;
	}

	iplen = strlen(ipaddr);
	printf("%s IP address: %s, len: %d\n", p_interface, ipaddr, iplen);

	/* Compiles the filter expression into a BPF filter program */ 
	if (pcap_compile(descr, &filter, filter_exp, 1, mask) == -1){
		fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
		return -1;
	}

	/* Load the filter program into the packet capture device. */ 
	if (pcap_setfilter(descr, &filter) == -1){
		fprintf(stderr, "ERROR setfilter fail: %s\n", pcap_geterr(descr) );
		return -1;
	}

        /* Compiles the filter expression into a BPF filter program */
        if (pcap_compile(descr, &filter, "arp", 1, mask) == -1){
                fprintf(stderr, "ERROR compile fail: %s\n", pcap_geterr(descr) );
                return -1;
        }

        /* Load the filter program into the packet capture device. */
        if (pcap_setfilter(descr, &filter) == -1){
                fprintf(stderr, "ERROR setfilter fail: %s\n", pcap_geterr(descr) );
                return -1;
        }


	//firstly close the old pcap fd;
	if(sockPtr->pcap)
		pcap_close(sockPtr->pcap);
	
	sockPtr->pcap = (void *)descr;

	return 0;
}
/* EOF */
