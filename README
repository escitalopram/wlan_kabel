# WLAN_Kabel

Connecting a computer without WLAN to a WLAN access point

## Introduction

Wireless network clients don't have bridging capabilities, because the network adapter cannot change it's MAC-address. However, it is possible to "bridge" a single computer over Wireless, by not using the wireless adapter locally and forwarding all the packet to a destination computer.

WLAN_Kabel implements this use case. It allows to connect a computer without any Wireless adapters to a Wireless LAN using a "proxy" device, such as a netbook, equipped with both Ethernet and Wireless.

## Usage

You can compile WLAN_Kabel by just entering "make". You will probably need make, a c-compiler and kernel headers installed.

To use WLAN_Kabel, you have to install it on the proxy device. Both the Wireless and the Ethernet adapter must be up, but not configured for any IP addresses. You can then start as root it like this:

`# ./wlan_kabel <wlan-adapter> <ethernet-adapter> <MAC of device to connect>`

Example:

`\# ./wlan_kabel wlan0 eth0 00:22:15:49:e5:55`

You can then connect your computer to the proxy device with a patch cable and bring it up normally - it will work with DHCP, etc.

WLAN_Kabel has been tested on an ATH9K chipset with Ubuntu 10.10

## Limitations

WLAN_Kabel will only work for one single network device. It will not work with multiple devices, not even with virtualized ones.

## License

You may use this code under the conditions of the GNU GPL v2 or later.

