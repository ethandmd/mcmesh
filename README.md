# mcmesh
This program is designed to listen to packets 802.11 "in the air".
To run this program you will need:
- Linux. 
- A wifi chipset that:
    - has a driver which can support nl80211 (ref. [drivers](https://wireless.wiki.kernel.org/en/users/drivers)).
    - is capable of supporting a monitor mode interface (this program can detect if you have one or not).
- [Dependencies](#Install-Dependencies)


## Build & Run
(Makefile under construction.)
```
$ gcc nl_utilities.c mcpcap.c start_mntr.c -o start_mntr $(pkg-config --cflags --libs libnl-genl-3.0)
$ sudo ./start_mntr {number of packets to capture}
```

### Install-Dependencies
We need to install [libnl](https://www.infradead.org/~tgr/libnl/) with headers for dev (or in the future possibly go to libmnl, libnl tiny (OpenWRT)) in order to use the ```nl80211.h``` enums.
Most linux distributions have libnl via their respective package managers.

+ Example:
```
$ sudo apt-get install libnl-genl-3-dev
```

    
 
#### Reference
+ Libnl: [Libnl developer docs](https://www.infradead.org/~tgr/libnl/doc/core.html)
+ iw (J. Berg.): [iw git repo](http://git.kernel.org/?p=linux/kernel/git/jberg/iw.git)
+ Netlink: [RFC 3549](https://datatracker.ietf.org/doc/html/rfc3549) 
+ Driver for Alfa wireless chipset (RTL8812au) for RPI3 from aircrack-ng:
```
$ git clone -b v5.6.4.2 https://github.com/aircrack-ng/rtl8812au.git
$ cd rtl*
$ sudo apt-get install raspberrypi-kernel-headers
#$ sed -i 's/CONFIG_PLATFORM_I386_PC = y/CONFIG_PLATFORM_I386_PC = n/g' Makefile    #No workie
$ make && sudo make install   #then reboot
```