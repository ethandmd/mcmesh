# $ gcc start_monitor.c nl_utilities.c wpcap.c mcpcap.c -o start_monitor $(pkg-config --cflags --libs libnl-genl-3.0) -lpcap

CC=gcc
#LDFLAGS=$(shell pkg-config --libs libnl-genl-3.0)
LIBS=-lnl-genl-3 -lnl-3
CFLAGS=-Wall -Werror -Wpedantic #-Wno-unused-parameter
CFLAGS += $(shell pkg-config --cflags libnl-genl-3.0)
#OBJ=start_monitor.o nl_utilities.o mcpcap.o

all: start_monitor #test_nl_utilities

# test_nl_utilities: nl_utilities.o test_nl_utilities.o
# 	${CC} ${CFLAGS -o $@ -c $< ${LDFLAGS} 

start_monitor: nl_utilities.o mcpcap.o start_monitor.o
	${CC} -o $@ -c $< ${CFLAGS} ${LIBS}

# test: all
# 	./test_nl_utilities 

clean:
	rm -rf *.o