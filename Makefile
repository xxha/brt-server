#CC=gcc
VERSION=\"1.1.0\"
AUTHOR=\"VEEX\"
TFTP_BOOT_DIR=/tftpboot
CC=arm-none-linux-gnueabi-gcc
#CFLAG=-I/usr/local/include -DDROUTE -c 
CFLAG=-I/home/xxha/tools/pcap/arm_lib_install/include -DDROUTE -c
#LFLAG=-L/usr/local/lib -lpcap -lpthread
LFLAG=-L/home/xxha/tools/pcap/arm_lib_install/lib -lpcap -lpthread
all:brt-server bindrt-test
#OBJ=main.o route.o arp.o ip.o pub.o packet_monitor.o ipc_monitor.o 
OBJ=main.o route.o ipc_monitor.o droute.o

brt-server:$(OBJ)
	$(CC) $^ $(LFLAG) -o $@
	cp -v $@ $(TFTP_BOOT_DIR)
	cp brt-bump.sh $(TFTP_BOOT_DIR)
	#sz $@
arp.o:arp.c arp.h pub.h
	$(CC) $< $(CFLAG) 
ip.o:ip.c ip.h
	$(CC) $< $(CFLAG) 
main.o:main.c pub.h
	$(CC) $< $(CFLAG) -DVERSION=$(VERSION) -DAUTHOR=$(AUTHOR) 
route.o:route.c route.h pub.h
	$(CC) $< $(CFLAG) 
#pub.o:pub.c pub.h
#	$(CC) $< $(CFLAG) 
scan_dev_name.o:scan_dev_name.c scan_dev_name.h main.h
	$(CC) $< $(CFLAG) 
packet_monitor.o:packet_monitor.c packet_monitor.h 
	$(CC) $< $(CFLAG) 
ipc_monitor.o:ipc_monitor.c ipc_monitor.h
	$(CC) $< $(CFLAG) 
droute.o:droute.c 
	$(CC) $< $(CFLAG) 

	
bindrt-test:bindrt-test.o  libbindrt.a
	$(CC)  -o $@ $< -L . -l bindrt 
	cp -v bindrt-test $(TFTP_BOOT_DIR)
	#sz $@
libbindrt.a:bindrt.o bindrt.h msgque.h
	ar -rc $@  $<
bindrt-test.o:bindrt-test.c bindrt.h
	$(CC) $< $(CFLAG) 
bindrt.o:bindrt.c bindrt.h msgque.h
	$(CC) $< $(CFLAG) 
clean:
	rm -fr $(OBJ) route-bind bindrt-test brt-server libbindrt.a bindrt.o bindrt-test.o

