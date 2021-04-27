
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Debug.h"
#include "Network.h"

bool tcpInit(){			//only RTSP will use TCP

	server->socketfd = socket(AF_INET,SOCK_STREAM, 0);
	if (server->socketfd == -1) goto error;

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server->RTSP_port);
	server_addr.sin_addr.s_addr = inet_addr(server->IP);
	
	int sinsize = 1;
	if (setsockopt(server->socketfd, SOL_SOCKET, SO_REUSEADDR, 
		&sinsize, sizeof(int)) == -1)
		goto error;
	
	if (bind(server->socketfd,(struct sockaddr *)&(server_addr), 
		sizeof(struct sockaddr)) == -1)
		goto error;
	
	if (listen(server->socketfd, 50) == -1)	goto error;

	return true;

error:
#ifdef DEBUG
	perror("");
#endif
	return false;
}

int tcpSend(Client *client) {		//only RTSP response will use TCP send

	ssize_t num = send(client->RTSP_msg->RTSP_fd, (void *)client->RTSP_msg->response_packet, \
		strlen(client->RTSP_msg->response_packet), 0);
		
#ifdef DEBUG	
	if(num == -1) perror("tcp send()");
#endif

	return num;
}

int tcpRecv(Client *client) {		//only RTSP receive will use TCP recv
	
	memset(client->RTSP_msg->income_packet, 0, RTSP_BUFFER_SIZE);
	ssize_t num = recv(client->RTSP_msg->RTSP_fd, (void *)client->RTSP_msg->income_packet, \
		RTSP_BUFFER_SIZE, 0);
		
#ifdef DEBUG
	if(num == -1) perror("tcp send()");
#endif

	return num;
}

int find_port() {
	/*
	int fd1 = socket(AF_INET, SOCK_STREAM, 0);
	int fd2 = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in addr1;
	struct sockaddr_in addr2;
	memset(&addr1, 0, sizeof(struct sockaddr_in));
	memset(&addr2, 0, sizeof(struct sockaddr_in));
	addr1.sin_family = AF_INET;
	addr2.sin_family = AF_INET;
	
	//从端口6000开始找，少用
	int port;
	for(port = 6000; port < 65535; port+=2){
		addr1.sin_port = htons(port);
		addr2.sin_port = htons(port+1);
		inet_pton(AF_INET, "0.0.0.0", &addr1.sin_addr);
		inet_pton(AF_INET, "0.0.0.0", &addr2.sin_addr);
		if(bind(fd1, (struct sockaddr *)(&addr1), sizeof(struct sockaddr_in)) < 0) {
		#ifdef DEBUG
			printf("port %d has been used.\n", port);
		#endif
			continue;
		} else if(bind(fd2, (struct sockaddr *)(&addr2), sizeof(struct sockaddr_in)) < 0) {
		#ifdef DEBUG
			printf("port %d has been used.\n", port+1);
		#endif
			continue;
		} else {
			close(fd1);
			close(fd2);
			break;
		}
		
	}
	*/
	static int port = 5998;
	port += 2;
	return port;
}

bool udpInit(media_class_t *media_class, char *IP) {		//only RTP/RTCP will use UDP
	
	media_class->RTP_msg->RTP_fd  = socket(AF_INET, SOCK_DGRAM, 0);
	//media_class->RTP_msg->RTCP_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (media_class->RTP_msg->RTP_fd < 0 ) {		//|| media_class->RTP_msg->RTCP_fd < 0
		media_class->RTP_msg->RTP_fd  = -1;
		//media_class->RTP_msg->RTCP_fd = -1;
		perror("udp socket()");
		return false;
	}
	
	media_class->RTP_msg->RTP_addr.sin_family  = AF_INET;
	//media_class->RTP_msg->RTCP_addr.sin_family = AF_INET;
	media_class->RTP_msg->RTP_server_port  = find_port();
	//media_class->RTP_msg->RTCP_server_port = media_class->RTP_msg->RTP_server_port + 1;
	media_class->RTP_msg->RTP_addr.sin_port  = htons(media_class->RTP_msg->RTP_client_port);
	//media_class->RTP_msg->RTCP_addr.sin_port = htons(media_class->RTP_msg->RTCP_client_port);
	media_class->RTP_msg->RTP_addr.sin_addr.s_addr  = inet_addr(IP);
	//media_class->RTP_msg->RTCP_addr.sin_addr.s_addr = inet_addr(IP);
	
	int sinsize = 1;
	if(setsockopt(media_class->RTP_msg->RTP_fd, SOL_SOCKET, SO_REUSEADDR, \
		&sinsize, sizeof(int)) == -1){
		perror("RTP setsockopt()");
		return false;
	}
	/*if(setsockopt(media_class->RTP_msg->RTCP_fd, SOL_SOCKET, SO_REUSEADDR, \
		&sinsize, sizeof(int)) == -1){
		perror("RTCP setsockopt()");
		return false;
	}*/
	
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family  = AF_INET;
	saddr.sin_port  = htons(media_class->RTP_msg->RTP_server_port);
	saddr.sin_addr.s_addr  = inet_addr(server->IP);
	
	if (bind(media_class->RTP_msg->RTP_fd,(struct sockaddr *)&saddr, sizeof(struct sockaddr)) == -1){
		perror("udp bind()");
		return false;
	}
	
	return true;
}

int udpSendto(media_class_t *media_class, int len) {		//only RTP will use UDP sendto

	ssize_t num = sendto(media_class->RTP_msg->RTP_fd, media_class->RTP_msg->RTP_buf, 
		len, 0, (struct sockaddr *)&(media_class->RTP_msg->RTP_addr), 
		sizeof(struct sockaddr));
	if (num != len){
	#ifdef DEBUG
		printf("Warning: %s err. %d %d\n", __FUNCTION__, (uint32_t)num, len);
	#endif
		return -1;
	}

	return num;
}

/*
int udpRecvfrom(Client *client) {	//only RTCP will use UDP recvfrom
	
	int addr_len = sizeof(struct sockaddr);
	ssize_t num = recvfrom(client->RTP_msg->RTCP_fd, client->RTP_msg->RTCP_buf, \
		MTU, 0, (struct sockaddr *)&(client->RTP_msg->RTCP_addr), \
		&addr_len);
	if (num == -1){
		perror("recvfrom()");
		return -1;
	}

	return num;
}*/
