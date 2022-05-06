# CC=gcc
# CFLAGS=-Wall -Wextra -Wpedantic #-Werror
# LDFLAGS=$(CFLAGS) $(shell pkg-config --libs libnl-genl-3.0)
# OBJ=$(STC:.c=.o)
# CFLAGS+=$(shell pkg-config --cflags libnl-genl-3.0)

# all: nl_utilities mcpcap sample

# sample: nl_utilities.o mcpcap.o sample.o
# 	$(CC) $(LDFLAGS) -o $@ $^

# %.o: %.c %.h
# 	$(CC) $(CFLAGS) -c -o $@ $<

# clean:
# 	rm -rf *.o nl_utilities mcpcap sample