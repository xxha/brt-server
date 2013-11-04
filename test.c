#include <stdio.h>

unsigned char buf[2048];

int main(int argc, char *argv[])
{
	FILE *file;
	int ret;

	printf("one\n");
	system("route del -host 192.168.8.234 >/dev/null 2>&1");

	printf("two\n");
	system("route del -host 192.168.8.234 >/dev/null");

	return 0;
}
