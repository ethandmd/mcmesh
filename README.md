# mcmesh

## Set up monitor mode pcap with RTL8812au on RPI3B+:
```
$ git clone -b v5.6.4.2 https://github.com/aircrack-ng/rtl8812au.git
$ cd rtl*
$ sudo apt-get install raspberrypi-kernel-headers
#$ sed -i 's/CONFIG_PLATFORM_I386_PC = y/CONFIG_PLATFORM_I386_PC = n/g' Makefile    #No workie
$ make && sudo make install

$ iw dev | grep "Interface"
$ sudo ip link set IFNAME down
$ sudo iw dev IFNAME set type monitor
$ sudo ip link set IFNAME up
```