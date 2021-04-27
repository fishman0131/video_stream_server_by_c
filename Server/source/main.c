
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "RTSP_Session.h"
#include "Server_Client.h"
#include "Network.h"
#include "Media_type.h"

int main(int argv, char **argc)
{
	struct sockaddr_in caddr;
	memset(&caddr, 0, sizeof(struct sockaddr_in));
	socklen_t size = sizeof(struct sockaddr);
	
	if (!server_init()) {
		perror("");
		exit(1);
	}
	
	while (1){
		int newsock = accept(server->socketfd, (struct sockaddr *)&caddr, &size);
		if(newsock != -1)
			if(client_locate(&caddr)) {
				Client *client = client_init(&caddr, newsock);
				if(pthread_create(&(client->RTSP_msg->RTSP_tid), NULL, RTSP_Session_Process, client) != 0) {
					perror("pthread_create()");
				}
			}
	}
	
	return 0;
	
}