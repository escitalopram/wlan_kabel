/* WLAN_Kabel (c) by Wolfgang Illmeyer
 * You may use this code under the conditions
 * of the GNU GPL v2 or later (see LICENSE)
 */


#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<netpacket/packet.h>
#include<netinet/ether.h>
#include<net/ethernet.h>
#include<linux/if.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>

// MAC adresses of some interfaces
static unsigned char destmac[6];
static unsigned char wlanmac[6];

// Packet Sockets
static int seth;
static int swlan;

// Interface indices
static int ethi;
static int wlani;

// return interface index of given network interface
static int retrieveifindex(char*name) {
	int sock = socket( AF_INET , SOCK_DGRAM , 0 );
	struct ifreq karl;
	strncpy(karl.ifr_name,name,IFNAMSIZ);
	ioctl(sock,SIOCGIFINDEX,&karl);
	close(sock);
	return karl.ifr_ifindex;
}

static void printmac(void* src, char* title) {
	unsigned char* karl= src;
        printf("%s: %02x:%02x:%02x:%02x:%02x:%02x\n",title,karl[0],karl[1],karl[2],karl[3],karl[4],karl[5]);
}

// parse MAC address from str to dest
static void parsemac(char* str, void* dest) {
        unsigned int karl[6];                                                                                                                                                                
	int i;
        sscanf(str,"%02x:%02x:%02x:%02x:%02x:%02x",&karl[0],&karl[1],&karl[2],&karl[3],&karl[4],&karl[5]);                                                                               
        printf("%02x:%02x:%02x:%02x:%02x:%02x\n",karl[0],karl[1],karl[2],karl[3],karl[4],karl[5]);                                                                                           
	for (i=0;i<6;++i) ((unsigned char*)dest)[i]=(unsigned char)karl[i];
}

// save MAC address of interface iface to dest
static void readmac(char* iface, void* dest) {
	// int ifindex = retrieveifindex(s,iface);
	struct ifreq karl;
	int i;
	int s = socket( AF_INET , SOCK_DGRAM , 0 ); 
	bzero(&karl,sizeof(karl));
	strncpy(karl.ifr_name,iface,IFNAMSIZ);
	perror(karl.ifr_name);
	ioctl(s,SIOCGIFHWADDR,&karl);
	unsigned char * dp = dest;
	for(i=0;i<6;++i) {
		dp[i]=karl.ifr_hwaddr.sa_data[i];
	}
	close(s);
}

// Create packet socket
static int get_rawsocket(char* iface, int socktype) {
        struct sockaddr_ll b;
        struct packet_mreq mr;
        int i;
	bzero(&b,sizeof(b));
        b.sll_family=AF_PACKET;
        b.sll_protocol=htons(ETH_P_ALL);
        bzero(&mr,sizeof(mr));  
        int swlan = socket(PF_PACKET,socktype,htons(ETH_P_ALL));

        int ifindex = retrieveifindex(iface);
        b.sll_ifindex=ifindex;
        b.sll_protocol=htons(ETH_P_ALL);
        int err = bind(swlan,(const struct sockaddr *)&b,sizeof(b));
        if (err!=0) {
                perror("error bind");
		exit(-1);
        }
        mr.mr_ifindex=ifindex;
        mr.mr_type=PACKET_MR_PROMISC;
        if (setsockopt(swlan,SOL_PACKET,1,&mr,sizeof(mr))!=0) {
                perror("error setsockopt");
		exit(-1);
        }
        if (fcntl(swlan,F_SETFL,O_NONBLOCK)==-1) {
                perror("error nonblock");
		exit(-1);
        }
	return swlan;
}

// check if forwarding a packet from ethernet is allowed
static int is_forwardable_eth(char * buf, int len) {
	if (memcmp(destmac,&buf[6],6)!=0) {
		printmac(&buf[6],"Dropping packet from unauthorized host");
		return 0;
	}
	return 1;
}


// check if ARP-packet, adjust packet data accordingly
static void adjust_arp(char * buf, int len) {
	int check=htons(ETH_P_ARP);
	if (memcmp(&check,&buf[12],2)!=0) return;
	if (buf[20]!=0 || !( buf[21]==2 || buf[21]==1)  ) return;
	memcpy(&buf[22],wlanmac,6);
}

// Forward packet received from WLAN (WLAN uses cooked mode)
static void forward_packet_wlan() {
	int i;
	unsigned char buf[65536];
	struct sockaddr_ll sinfo;
	bzero(&sinfo,sizeof(sinfo));
	int froml=sizeof(sinfo);
	int mylen = recvfrom(swlan,buf+14,sizeof(buf)-14,0,(struct sockaddr*)&sinfo,&froml);
	if (mylen<0) {
		perror("read");
		exit(-1);
	}
	for (i=0;i<6;++i) buf[i]=destmac[i];
	for (i=0;i<6;++i) buf[i+6]=sinfo.sll_addr[i];
	memcpy(&buf[12],&(sinfo.sll_protocol),2);
	int sendlen = send(seth,buf,mylen+14,0);
	if(sendlen<0) {
		perror("send");
		exit(-1);
	}
}

// Forward packet revceived via ethernet (includes L2-headers)
static void forward_packet_eth() {
	int i;
	unsigned char buf[65536];
	struct sockaddr_ll sinfo;
	bzero(&sinfo,sizeof(sinfo));
	int froml=sizeof(sinfo);
	int mylen = recvfrom(seth,buf,sizeof(buf),0,(struct sockaddr*)&sinfo,&froml);
	if (mylen<0) {
		perror("read");
                	exit(-1);
	}
	if (is_forwardable_eth(buf,mylen)) {
		adjust_arp(buf,mylen);
		memcpy(&(sinfo.sll_addr),buf,6);
		sinfo.sll_ifindex=wlani;
		int sendlen = sendto(swlan,&buf[14],mylen-14,0,(const struct sockaddr *)&sinfo,sizeof(sinfo));
		if(sendlen<0) {
			perror("send");
			exit(-1);
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc!=4) {
		printf("error: need 3 arguments: <wlan_if> <lan_if> <dest_mac>");
	}

	swlan = get_rawsocket(argv[1],SOCK_DGRAM);
	seth = get_rawsocket(argv[2],SOCK_RAW);
	ethi = retrieveifindex(argv[2]);
	wlani = retrieveifindex(argv[1]);
	parsemac(argv[3],destmac);
	readmac(argv[1],wlanmac);
	printmac(destmac,"Destination MAC");
	printmac(wlanmac,"WLAN MAC");

	while(1){
		fd_set fsr;
		FD_ZERO(&fsr);
		FD_SET(swlan,&fsr);
		FD_SET(seth,&fsr);
		select(seth+swlan+1,&fsr,0,0,0);
		if(FD_ISSET(swlan,&fsr)) forward_packet_wlan();
		if(FD_ISSET(seth,&fsr)) forward_packet_eth();
	}
}
