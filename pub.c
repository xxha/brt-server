#include <stdio.h>

void print_buf(unsigned char *buf,int size,int col){
	int i,j;

	for(i=0,j=0;i<size;i++,j++){
		if(j==col){
			printf("\n");
			j=0;
		}
		printf("0x%02X ",buf[i]);
		
	}
	printf("\n");
}
