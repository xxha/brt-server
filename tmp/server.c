#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <ctype.h>
#include <string.h>
unsigned buf[10240];
int main(int argc,char *argv[]){
	mqd_t mq_receive,mq_send;
	int ret,i;
	mq_receive=mq_open("/mq_brt_server_que_one",O_RDONLY|O_CREAT,S_IRUSR|S_IWUSR,NULL);
	if((mqd_t )-1==mq_receive){
		perror("dsf");
		return -1;
	}
	mq_send=mq_open("/mq_brt_server_que_two",O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR,NULL);
	if((mqd_t )-1==mq_send){
		perror("dsf");
		return -1;
	}
	
	memset(buf,0x00,sizeof(buf));
	ret=mq_receive(mq_receive,buf,sizeof(buf),NULL);
	if(ret!=-1){
		printf("Received:%s\n",buf);
		for(i=0;i<ret;i++)
			buf[i]=toupper(buf[i]);
		ret=mq_send(mq_receive,buf,strlen(buf),NULL);
	}
	perror(NULL);
	mq_close(mq_receive);
	mq_close(mq_send);
	mq_unlink("/mq_brt_server_que_one");
	mq_unlink("/mq_brt_server_que_two");
	return 0;
}
