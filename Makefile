#CC=gcc
VERSION=\"1.2.0\"
AUTHOR=\"VEEX\"
TFTP_BOOT_DIR=/tftpboot

CC=arm-none-linux-gnueabi-gcc -Wall -O2 -I.
CFLAG=-I/home/xxha/tools/pcap/arm_lib/include -DDARP -c
LFLAG=-L/home/xxha/tools/pcap/arm_lib/lib -lpcap -lpthread

all:brt-server bindrt-test

OBJ=main.o route.o ipc_monitor.o darp.o

brt-server:$(OBJ)
	$(CC) $^ $(LFLAG) -o $@

parse.o:parse.c parse.h
	$(CC) $< $(CFLAG) 
main.o:main.c
	$(CC) $< $(CFLAG) -DVERSION=$(VERSION) -DAUTHOR=$(AUTHOR) 
route.o:route.c route.h
	$(CC) $< $(CFLAG) 
scan_dev_name.o:scan_dev_name.c scan_dev_name.h main.h
	$(CC) $< $(CFLAG) 
packet_monitor.o:packet_monitor.c packet_monitor.h 
	$(CC) $< $(CFLAG) 
ipc_monitor.o:ipc_monitor.c ipc_monitor.h
	$(CC) $< $(CFLAG) 
darp.o:darp.c
	$(CC) $< $(CFLAG) 


bindrt-test:bindrt-test.o  libbindrt.a
	$(CC)  -o $@ $< -L . -l bindrt 
libbindrt.a:bindrt.o bindrt.h msgque.h
	ar -rc $@  $<
bindrt-test.o:bindrt-test.c bindrt.h
	$(CC) $< $(CFLAG) 
bindrt.o:bindrt.c bindrt.h msgque.h
	$(CC) $< $(CFLAG) 
clean:
	rm -fr $(OBJ) route-bind bindrt-test brt-server libbindrt.a bindrt.o bindrt-test.o

