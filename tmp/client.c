#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <ctype.h>
#include <string.h>
int main(int argc,char *argv[]){
	mqd_t mq_id;
	int ret;
	unsigned char buf[10240];
	if(argc<2){
		printf("Usage:%s message\n",argv[0]);
		return -1;
	}
	mq_id=mq_open("/zxqmq",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR,NULL);
	if((mqd_t )-1==mq_id){
		perror(NULL);
		return -2;
	}
	strcpy(buf,argv[1]);
	ret=mq_send(mq_id,buf,strlen(buf),NULL);	
	if(0!=ret){
		perror(NULL);
		printf("Failed to send message.\n");
		mq_close(mq_id);
		return -3;
	}
	ret=mq_receive(mq_id,buf,sizeof(buf),NULL);
	if(ret!=-1){
		printf("Received:%s\n",buf);
	}
	mq_close(mq_id);
	mq_unlink("/tmp/mq");
	return 0;
}
