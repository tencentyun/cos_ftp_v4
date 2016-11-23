# Makefile for systems with GNU tools
CC 	=	gcc
CXX =	g++
INSTALL	=	install
IFLAGS  = -idirafter dummyinc
#CFLAGS = -g
CFLAGS	=	-g -O0 -fPIE -fstack-protector --param=ssp-buffer-size=4 \
	-Wall -W -Wformat-security -I../include -I../include/mysql -DBIG_JOINS=1  -fno-strict-aliasing \
	-D_FORTIFY_SOURCE=2 \
	#-pedantic -Wconversion -Wshadow 

LIBS	=	`./vsf_findlibs.sh`
#LINK	=	-Wl,-s 
#LDFLAGS	=	-fPIE -pie -Wl,-z,relro -Wl,-z,now
LINK	=	
LDFLAGS	= -L/usr/lib/x86_64-linux-gnu -L../lib -lcossdk -ljsoncpp -lcurl -lcrypto -lpthread -lz -lm -lrt -ldl -lidn -lglog

OBJS	=	main.o utility.o prelogin.o ftpcmdio.o postlogin.o privsock.o \
		tunables.o ftpdataio.o secbuf.o ls.o \
		postprivparent.o logging.o str.o netstr.o sysstr.o strlist.o \
    banner.o filestr.o parseconf.o secutil.o \
    ascii.o oneprocess.o twoprocess.o privops.o standalone.o hash.o \
    tcpwrap.o ipaddrparse.o access.o features.o readwrite.o opts.o \
    ssl.o sslslave.o ptracesandbox.o ftppolicy.o sysutil.o sysdeputil.o \
    seccompsandbox.o \
	cosswitch.o encode.o 

ifdef TEST
	ifeq ("$(origin TEST)", "command line")
		ifeq ("$(TEST)", "1")
			CFLAGS += -DTEST
		endif
	endif
endif

.c.o:
	$(CC) -c $*.c $(CFLAGS) $(IFLAGS)

.cc.o:
	$(CXX) -c $*.cc $(CFLAGS) $(IFLAGS)

.cpp.o:
	$(CXX) -c $*.cpp $(CFLAGS) $(IFLAGS)

vsftpd: $(OBJS) 
	$(CXX) -o vsftpd $(OBJS) $(LINK) $(LIBS) $(LDFLAGS) 

#install:
#	if [ -x /usr/local/sbin ]; then \
		$(INSTALL) -m 755 vsftpd /usr/local/sbin/vsftpd; \
	else \
		$(INSTALL) -m 755 vsftpd /usr/sbin/vsftpd; fi
#	if [ -x /usr/local/man ]; then \
		$(INSTALL) -m 644 vsftpd.8 /usr/local/man/man8/vsftpd.8; \
		$(INSTALL) -m 644 vsftpd.conf.5 /usr/local/man/man5/vsftpd.conf.5; \
	elif [ -x /usr/share/man ]; then \
		$(INSTALL) -m 644 vsftpd.8 /usr/share/man/man8/vsftpd.8; \
		$(INSTALL) -m 644 vsftpd.conf.5 /usr/share/man/man5/vsftpd.conf.5; \
	else \
		$(INSTALL) -m 644 vsftpd.8 /usr/man/man8/vsftpd.8; \
		$(INSTALL) -m 644 vsftpd.conf.5 /usr/man/man5/vsftpd.conf.5; fi
#	if [ -x /etc/xinetd.d ]; then \
		$(INSTALL) -m 644 xinetd.d/vsftpd /etc/xinetd.d/vsftpd; fi

clean:
	rm -f *.o *.swp vsftpd

