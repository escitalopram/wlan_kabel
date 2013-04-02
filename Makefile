all: wlan_kabel

wlan_kabel: wlan_kabel.c
	cc -o wlan_kabel wlan_kabel.c
