# mcmesh
This program is designed to sniff 802.11 frames "in the air" or ethernet frames from a wifi interface on a local network.
To run this program you will need:
- Linux. 
- A wifi chipset that:
    - has a driver which can support nl80211 (ref. [drivers](https://wireless.wiki.kernel.org/en/users/drivers)).
    - is capable of supporting a monitor mode interface (this program can detect if you have one or not).
- [Dependencies](#Install-Dependencies)

## Build & Run
```
$ make
$ sudo ./mcmesh -interface, --i {interface name} [--count, -c] {No. of packets to capture} [--type, -t] {capture interface type (i.e. "monitor")}
```
*For interface type, only supports station (managed) mode, or monitor mode.
Currently (until signal catching implementation is finished) if you kill the program during a monitor mode capture before it's finished (or if no --count was specified), it will not delete the monitor mode interface, and your prior existing wireless interface will not be restored. In this case, you would likely benefit from the following `iw` commands:
```
#Delete monitor interface:
$ sudo iw dev mcmon del
#Restore managed interface:
$ sudo iw phy {WIPHY (e.g. phy0)} interface add {IFACE NAME} type managed
```

### Install-Dependencies
1) Install [libnl](https://www.infradead.org/~tgr/libnl/) with headers for dev (or in the future possibly go to libmnl, libnl tiny (OpenWRT)) in order to use the ```nl80211.h``` enums.
Most linux distributions have libnl via their respective package managers.

+ Example (Ubuntu 21.04):
```
$ sudo apt install libnl-genl-3-dev
```
+ Example (Fedora 35):
```
$ sudo dnf install libnl3-devel
```

2) Install [libpcap](https://github.com/the-tcpdump-group/libpcap) (dev). After building up a version using just packet sockets, for the sake of development speed, it was significantly faster to rely on some of the pre-built ```pcap``` functions.

+ Example (Ubuntu 21.04):
```
$ sudo apt install lipcap-dev
```
+ Example (Fedora 35):
```
$ sudo dnf install libpcap-devel
```
 
#### Reference
+ Libnl: [Libnl developer docs](https://www.infradead.org/~tgr/libnl/doc/core.html)
+ iw (J. Berg.): [iw git repo](http://git.kernel.org/?p=linux/kernel/git/jberg/iw.git)
+ Libpcap nl80211 examples: [pcap-linux](https://github.com/the-tcpdump-group/libpcap/blob/master/pcap-linux.c)
+ Kismet Wireless [rf_kill linux](https://github.com/kismetwireless/kismet/blob/master/capture_linux_wifi/linux_wireless_rfkill.c)
+ Link layer headers from [libpcap](https://www.tcpdump.org/linktypes.html)
+ Documentation for [radiotap](https://radiotap.org)
+ Working with [radiotap](https://www.oreilly.com/library/view/network-security-tools/0596007949/ch10s03.html)
+ Wireshark [Capture Setup](https://gitlab.com/wireshark/wireshark/-/wikis/CaptureSetup/WLAN#linux)
+ Netlink: [RFC 3549](https://datatracker.ietf.org/doc/html/rfc3549) 
+ Driver for Alfa wireless chipset (RTL8812au) for RPI3 from aircrack-ng:
```
$ git clone -b v5.6.4.2 https://github.com/aircrack-ng/rtl8812au.git
$ cd rtl*
$ sudo apt-get install raspberrypi-kernel-headers
#$ sed -i 's/CONFIG_PLATFORM_I386_PC = y/CONFIG_PLATFORM_I386_PC = n/g' Makefile    #No workie
$ make && sudo make install   #then reboot
```

# Narrative
## Who
author: Ethan

collaborators:<br/>
Thomas (testing on Arch & code review), Henry (testing on Fedora). <br/>
*all testing happened on x86_64 machine with intel wifi cards, and raspberrypi 3/4 with RTL8812au USB Alfa Wireless chipset for monitor mode, standard Broadcomm chipset for regular capture.

## What
This program is a limited link layer packet capture mechanism written in C, relying on generic netlink and libpcap. There are two principle methods with which to run this program, either running packet capture mechanism on a network interface in managed mode (parse ethernet frames), or on a monitor mode wireless interface (parse 802.11 frames). For non-monitor mode packet capture, this program just blindly reads ethernet frames (granted using some conveniance of libpcap instead of my initial implementation with packet sockets), and prints the source's hardware address, destination's hardware address, and the (IP) Layer 3 protocol used, should further packet parsing be desired. Monitor mode capture is more interesting, and took more investigating to figure out (still not quite a 100% solution, frankly). Before I go any further, some background on the Linux wireless subsystem is required. Starting in 2009, Johannes Berg took control of the linux wifi stack, and implemented (roughly) the following:
```
            mcmesh              (user)
              |
===========nl80211============= ------------
              |                 (kernel)
           cfg80211
              |
           mac80211
              |
        Drivers (iwlwifi,...)
```
[credit](https://wireless.wiki.kernel.org/_media/en/developers/documentation/mac80211.pdf).

So, if you are still using deprecated wireless management tools built on `wext` instead of [nl80211](https://wireless.wiki.kernel.org/en/developers/Documentation/nl80211) you should stop doing so! So, what is netlink, generally? [Netlink](https://linux.die.net/man/7/netlink) is a sockets-based interface for facilitating user space -- kernel space communication, and internal kernel space communication. Its prety cool, and interesting, and a large part of why I got into this project to begin with. As an IPC mechanism, it was primarily motivated to facilitate network related communication, and has expanded into subsets of further specialized netlink family sockets. However, noticeably, this project doesn't contain any netlink sockets outright, instead, this program uses [libnl](https://www.infradead.org/~tgr/libnl/). `libnl` provides an abstraction to use netlink, it's conveniant, and doesn't abstract much from just using an `AF_NETLINK` socket, it's also more or less the go-to for userspace programs using nl80211 (like this one). Next, `nl80211.h`, a header file with a predefined set of instructions used to communicate via netlink sockets to the wireless subsystem in kernel space. This is the center of the developer API for doing things like creating/deleting interfaces, setting wireless interface parameters, sending the wireless interface a command such as a scan for all nearby SSIDs. Needless to say, the bulk of ```mcmesh```'s `nl_utilities` rely on this API, asides from a few `ioctl` calls.

Ok, brief background on netlink and the linux wireless subsystem complete. Now a brief foray into 802.11, which isn't really something one can be brief about it. It is a nightmare of compatibility issues between hardware, drivers, IEEE standards, and just a myriad of problems that wouldn't exist if you only used elegant wired connections. For example, if you run this program on hardware with a hardmac vs. a softmac chipset, you likely won't have as much functionality in this context. Linked to above, `mac80211` is performing the MAC layer management (MLME) for softmac chips, as opposed to MLME happening on device for (full/hard)mac chip. So, when placed into monitor mode, there are several precautions one ought to take in order to get a "good" capture. First, it helps to have no other virtual interfaces on the same wireless physical device (wiphy), mostly because another interface could take precedence in associating with an access point (AP), or setting channel frequencies. 

Furthermore, I should delineate between promiscuous mode, and monitor mode. Promiscuous mode is when an interface remains associated with an AP, but doesn't filter out packets it receives that aren't addressed to it. For example, if you performed a packet capture while associated to your home network, but not in promiscuous mode, you would mostly just see traffic that, by convention, your interface needs to see. Promiscuous mode means that the filter comes off, and (mostly) ALL of the traffic on the associated network comes through. But, monitor mode is even more powerful, (passive) monitor mode means that the wireless interface is no longer associated to an AP, and will pass along ANY packet it happens to receive over the air. Now, since that is an overwhelming amount of traffic, you can apply Berkely Packet Filters (BPF) to compile a filter to screen packets before passing them up to user space, as you can imagine, sending EVERY packet up to user space before determining whether or not to keep it is unsatisfactory. However, BPFs are not very web2.0, so to speak, and the syntax is arcane and confusing, so libpcap in all its glory has a method to set and compile a BPF for you! 

Once an interface is in monitor mode, you can set its frequency, channel width, and other typical phy parameters to help refine the capture. This program does not (yet) have a robust channel hopping mechanism, however that ought to be coming! Finally, the most limited aspect of this program is the actual parsing of the 802.11 frames. Parsing the raw data, doing bit manipulation and type casting from binary to (structured-ish) data is tough because there are so many possible events in the sample space. For instance, each 802.11 frame header has a frame control field where we find out
+ a version (`0x0`)
+ a type (management, control, or data (sent as QoS as of late...?))
+ a subtype (e.g. management: {probe, beacon,...})
+ whether it is `TO DS` to a distribution system
+ wether it is `FROM DS` from a distrubution system
+ and 6 more bits of information, like power mode, etc...

Where flags like `TO DS` and `FROM DS` indicate very important things about the status of the four possible hardware addresss fields in the frame header! If each type has upwards of ten subtypes, then you can imagine that the amount of labor and IEEE standard-reading required to parse any possibly arriving 802.11 frame is more than the scope of this project. Instead, when in monitor mode, this program uses the `nl80211` API to avoid control frames, and can compile BPF to only capture management frames, furthermore, only beacon, or probe request frames. So, instead, we focus on a limited assortment of 802.11 frame types, and in a loose sense of the term, leave the rest as "an exercise for the reader" :/ . 

## Why
I am interested in 802.11 communication standards, specifically 802.11s implementations. Programming in C isn't something I've done before (wrt modern C++), and I've never worked with something beyond a websocket. So, I am interested in two areas going forward: UNIX IPC mechanisms, exploring user/kernel space communication, and wireless mesh networking, specifically for what I'll call low-capability devices. Hence, these two interests certainly overlap, and in order to move forward on a more ambitious project I absolutely needed to break the ice with netlink, C (although languages like Go and Rust seem to have a lot of cool stuff going on here), and getting my hands on some raw 802.11 data. In the github repository for this project I have a few open issues that I would like to continue exploring, and they range in topic from efficiency, IPC exploration, and 802.11 comprehension. I think a 'nice-to-have' would be a front-end for this program, something browser based that could serve requests to hosts on the local network, that way you could have, say, a raspberry pi running this program and just tune in via web browser from your laptop instead of ssh. Beyond that, there are some key functionality flaws in this program. As I went through a few iterations of playing with nl80211 and libnl, I had originally anticipated the program having the ability to scan all wiphys available and find a monitor-mode capable one. While I was able to implement this, once I started testing it on other people's hardware, and with different Linux distros (Fedora, Arch, ...) I immediately started encountering more bugs than I was willing to deal with. So instead I leave it up it up to the user to supply an appropriate *interface* to capture with. 

## How
Here is an example control flow for capturing in monitor mode with this program:

```
cli arg: keep_name = INTERFACE NAME

/* Interface Setup (all through nl80211 commands) */
Store the interface configuraton for $(keep_name)
Create a new interface called mcmon on the same *phy* as $(keep_name) ... error checking for whether monitor mode is supported or not
Delete $(keep_name)
(Optional) Set the channel freq on mcmon

/* Capture Setup (no longer uses packet socket implementation, libpcap instead) */
Start libpcap 'session' bound to mcmon
Compile BPF expression (or leave as "" for kernel packets)

/* Capture */
Start loop to handle incoming packets

/*
 * TODO: Catch a CTRL-C signal and cleanup resources before exiting.
 */

/* Cleanup */
Delete mcmon interface
Re-create $(keep_name) interface with original config
Free netlink resources
Free libpcap resources
```