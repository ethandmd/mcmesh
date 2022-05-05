# mcmesh
Use netlink and a monitor-mode capable wifi chipset to scan nearby wifi base stations on raspberry pi.

## Set up:
### Get driver for RTL8812au on RPI3B:
```
$ git clone -b v5.6.4.2 https://github.com/aircrack-ng/rtl8812au.git
$ cd rtl*
$ sudo apt-get install raspberrypi-kernel-headers
#$ sed -i 's/CONFIG_PLATFORM_I386_PC = y/CONFIG_PLATFORM_I386_PC = n/g' Makefile    #No workie
$ make && sudo make install   #then reboot
```

### Get netlink on RPI3B:
```
$ sudo apt-get install libnl-genl-3-dev
```
We need to install libnl (or in the future possibly go to libmnl, libnl tiny (OpenWRT)) in order to use the ```nl80211.h``` enums.

### Guide to this universe:

#### Capture Goal:
+ Want to capture all nearby wireless traffic in the 2.4-5 GHz range.
    + Delete all interfaces from net device, create a new virtual interface in monitor mode.
    + Recall:
        + Monitor mode: Capture all packets in the air, not associated to AP.
        + Promiscuous mode: Capture all packets associated to AP.

#### Netlink: (Code mostly referenced from ```iw```, this seems to be the canonical way to learn nl80211 api + netlink).
+ Create & allocate a netlink socket: ```struct nl_sock *sk = nl_socket_alloc();```
+ Destroy & deallocate netlink socket: ```nl_socket_free(sk);```
+ Set socket attributes: modify callback, buffer size, auto-ack, etc...
+ Create fd & bind socket: ```genl_connect(sk);```
+ Send messages (auto complete): ```nl_send_auto(sk, msg);```
+ Recv messages (easy way): ```nl_recvmsgs_default(sk);```
    + Gets CB stored in socket and calls ```nl_recvmsgs(sk, cb);```
+ Send messages:
    + Allocate msg: ```struct nl_msg *msg = nlmsg_alloc();```
+ Set message callback functions with: 
    + ```nl_cb_set(struct nl_cb *cb, enum nl_cb_type type, enum nl_cb_kind kind, nl_recvmsg_cb_t func, void *arg)```
    + Error msg cb hook: ```nl_cb_err()```

#### Reference
+ Netlink Protocol: [libnl developer docs](https://www.infradead.org/~tgr/libnl/doc/core.html)
+ iw (J. Berg.): [iw git repo](http://git.kernel.org/?p=linux/kernel/git/jberg/iw.git)
+ [RFC 3549](https://datatracker.ietf.org/doc/html/rfc3549) 