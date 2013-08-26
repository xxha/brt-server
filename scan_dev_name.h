#ifndef __SCAN_DEV_NAME_H
#define __SCAN_DEV_NAME_H
#define DEV_NAME_MAX_SIZE 32
#define MAX_DEV_ORG_COUNTS 3
typedef struct{
	unsigned char dev_org_name[DEV_NAME_MAX_SIZE];
	unsigned char dev_cur_name[DEV_NAME_MAX_SIZE];
	unsigned char dev_new_name[DEV_NAME_MAX_SIZE];
}DEV_NAME_MAP;
void *scan_net_dev_name(void *para);
#endif
