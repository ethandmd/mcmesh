# $ gcc mcmesh.c nl_utilities.c wpcap.c handle80211.c -o mcmesh $(pkg-config --cflags --libs libnl-genl-3.0) -lpcap

CC = gcc
INCLUDE = -I/usr/include/libnl3
LDFLAGS = -lpcap -lnl-genl-3 -lnl-3
CFLAGS = -Wall -Werror -Wpedantic 

all: mcmesh

mcmesh: mcmesh.c nl_utilities.c wpcap.c handle80211.c
	$(CC) $(CFLAGS) mcmesh.c nl_utilities.c wpcap.c handle80211.c -o mcmesh $(INCLUDE) $(LDFLAGS)

clean:
	rm -rf *.o