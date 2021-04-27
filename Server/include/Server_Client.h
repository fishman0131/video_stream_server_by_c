
#ifndef _SERVER_CLIENT_H_
#define _SERVER_CLIENT_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>

#include "list.h"
#include "Debug.h"
#include "Media_type.h"

#define MTU 1456
#define RTSP_BUFFER_SIZE 4096
#define RTSP_Port 8554

typedef struct {	//Server
	
	char name[50];
	char IP[20];
	unsigned RTSP_port;
	unsigned client_num;
	
	int socketfd;
	
} Server;

typedef struct {	//RTSP_msg_t

	unsigned RTSP_port;
	int RTSP_fd;
	int method;
	char URI[50];
	char version[10];
	char CSeq[10];
	unsigned OurSessionId;
	
	char *income_packet;
	char *response_packet;
	
	pthread_t RTSP_tid;
	
} RTSP_class_t;

typedef struct {	//RTP_class_t
	
	uint16_t RTP_server_port;
	//uint16_t RTCP_server_port;
	uint16_t RTP_client_port;
	uint16_t RTCP_client_port;
	
	int RTP_fd;			//socket
	int RTCP_fd;
	struct sockaddr_in RTP_addr, RTCP_addr;
	uint8_t RTP_buf[MTU];
	uint8_t *load;		//RTP封包时用到的偏移指针
	
	int aggregation;	// 0: Single Unit, 1: Aggregation Unit
	unsigned short sequence_Basenumber;
	unsigned int Timestamp;
	unsigned int ssrc;
	
	pthread_t RTP_tid;
	
} RTP_class_t;

typedef struct {
	
	enum Media media_type;
	char media_name[50];
	uint8_t *media_buf;
	long media_size;
	
	unsigned framerate;			//视频帧率
	
	unsigned framesize;			//音频帧大小
	unsigned sample_rate;		//音频采样率
	
	RTP_class_t *RTP_msg;
	
	bool init_done;
	
} media_class_t;

typedef struct {	//Client
	
	char name[100];
	char IP[20];
	char filename[50];
	
	enum Media loading_media;
	enum { Birth, Init, Ready, Playing } state;
	
	struct timeval creation_time;
	struct list_head list;
	
	media_class_t *media_class[SMN];
	RTSP_class_t *RTSP_msg;
	
} Client;

Server *server;						//定义一个全局的Server
struct list_head client_list;		//定义了一个全局的struct list_head client_list为客户链表的索引

bool server_init();
Client *client_init(struct sockaddr_in *caddr, int newsock);
bool client_locate(struct sockaddr_in *caddr);
void client_insert_packet(Client *client, char *packet);
void client_realease(Client *client);
void client_realeaseAll(Client *client);
media_class_t *media_class_init(int media_type);

#endif