#include <stdio.h>
unsigned char buf[2048];
int main(int argc,char *argv[]){
	FILE *file;
	int ret;
#if 0
	file=fopen("/proc/net/route","r");
	if(NULL==file){
		printf("Failed to open ifle.\n");
		return 0;
	}
	
	ret=fread(buf,sizeof(buf),1,file);
	printf("%s\n",buf);

	fclose(file);


	if(isdigit('a'))
		printf("is");
	else
		printf("no");
	printf("\n");
#endif
	printf("one\n");
	system("route del -host 192.168.8.234 >/dev/null 2>&1");
	printf("two\n");
	system("route del -host 192.168.8.234 >/dev/null");
	return 0;
}
