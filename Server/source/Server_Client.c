
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "Debug.h"
#include "Server_Client.h"
#include "Network.h"
#include "RTP_transport.h"
#include "RTP_header.h"
#include "Media_type.h"

void 
print_welcome() 
{
	printf("\t\t---------Yearn Media Server---------\n\n");
	printf("Play streams from this server using the URL.\n");
	printf("\trtsp://%s:%d/<filename>\n", server->IP, RTSP_Port);
	printf("Only H.264 and AAC files are supported\n\n\n");
}

//初始化服务器
bool 
server_init()
{
	do{
		server = calloc(1, sizeof(Server));
		if(server == NULL) break;
		
		strcpy(server->name, "Yearn Media Server");
		
		//自动获取本机IP
		char MyIpBuf[32] = {0};
		FILE *fpRead;
		//网卡名称是enp2s0，其他PC可能是eth0等其他名字
		char* command=(char*)"ifconfig enp2s0|grep 'inet addr'|awk -F \":\" '{print $2}'|awk '{print $1}'";
		char* renewCh;
			
		fpRead = popen(command, "r");
		fgets(MyIpBuf, 32, fpRead);
		if(fpRead != NULL)
			pclose(fpRead);

		renewCh=strstr(MyIpBuf,"\r");
		if(renewCh)
			*renewCh='\0';
		renewCh=strstr(MyIpBuf,"\n");
		if(renewCh)
			*renewCh='\0';
		
		if(strlen(MyIpBuf) != 0) strcpy(server->IP, MyIpBuf);
		else break;
		
		//固定RTSP端口8554
		server->RTSP_port = RTSP_Port;
		
		//初始化连接的客户为零
		server->client_num = 0;
		
		//初始化Server的RTSP所用的TCP
		if(!tcpInit()) break;
		
		//初始化客户端链表头
		INIT_LIST_HEAD(&client_list);
		
		print_welcome();
		
	#ifdef DEBUG 
		printf("------------------------------------\n");
		printf("server init is done\n");
		printf("server IP: %s\n", server->IP);
		printf("server RTSP port: %d\n\n", server->RTSP_port);
	#endif
		
		return true;
		
	}while(0);
	
	return false;
}

media_class_t *
media_class_init(int media_type)
{
	media_class_t *media_class = calloc(1, sizeof(media_class_t));
	if(media_class == NULL) return NULL;
	media_class->RTP_msg = calloc(1, sizeof(RTP_class_t));
	if(media_class->RTP_msg == NULL) return NULL;
	
	strcpy(media_class->media_name, "null");
	media_class->media_type = media_type;
	media_class->media_buf = NULL;
	media_class->media_size = 0;
	media_class->framerate = 0;
	media_class->init_done = false;
	media_class->framesize = 1024;
	
	media_class->RTP_msg->RTP_server_port = 0;
	//media_class->RTP_msg->RTCP_server_port = 0;
	media_class->RTP_msg->RTP_client_port = 0;
	//media_class->RTP_msg->RTCP_client_port = 0;
	media_class->RTP_msg->RTP_fd = -1;
	//media_class->RTP_msg->RTCP_fd = -1;
	media_class->RTP_msg->aggregation = 0;	
	media_class->RTP_msg->load = media_class->RTP_msg->RTP_buf + sizeof(RTP_header);
	media_class->RTP_msg->Timestamp = 0;
	media_class->RTP_msg->ssrc = 0;
	media_class->RTP_msg->RTP_tid = 0;
	memset(&(media_class->RTP_msg->RTP_addr) , 0, sizeof(struct sockaddr_in));
	//memset(&(media_class->RTP_msg->RTCP_addr), 0, sizeof(struct sockaddr_in));
	
	return media_class;
}

//初始化客户端，并将其加入到客户端的内核链表
Client *
client_init(struct sockaddr_in *caddr, int newsock)
{
	Client *client = calloc(1,sizeof(Client));
	if(client == NULL) return NULL;
	client->RTSP_msg = calloc(1, sizeof(RTP_class_t));
	if(client->RTSP_msg == NULL) return NULL;
	client->RTSP_msg->income_packet = calloc(1, RTSP_BUFFER_SIZE);
	if(client->RTSP_msg->income_packet == NULL) return NULL;
	client->RTSP_msg->response_packet = calloc(1, RTSP_BUFFER_SIZE);
	if(client->RTSP_msg->response_packet == NULL) return NULL;
	
	++(server->client_num);
	strcpy(client->IP, inet_ntoa(caddr->sin_addr));
	//client->loading_media = Not_sure;
	client->state = Birth;
	gettimeofday(&client->creation_time, NULL);
	client->media_class[H264] = NULL;
	client->media_class[AAC] = NULL;
	
	client->RTSP_msg->method = -1;
	strcpy(client->RTSP_msg->CSeq, "\0");
	strcpy(client->RTSP_msg->version, "\0");
	strcpy(client->RTSP_msg->URI , "\0");
	client->RTSP_msg->OurSessionId = 0;
	client->RTSP_msg->RTSP_port = caddr->sin_port;
	client->RTSP_msg->RTSP_fd   = newsock;
	client->RTSP_msg->RTSP_tid = 0;
	
	list_add_tail(&(client->list), &client_list);
	
#ifdef DEBUG 
	printf("\n-----------------------------------------------------------\n");
	printf("There is client login\n");
	printf("client IP: %s\n", client->IP);
	printf("There are currently %d clients.\n\n", server->client_num);
#endif
	
	return client;
}

//媒体类释放函数
void 
media_class_realease(media_class_t *media_class)
{
	if(media_class->RTP_msg->RTP_tid != 0) 
		pthread_cancel(media_class->RTP_msg->RTP_tid);
	if(media_class->RTP_msg->RTP_fd != -1) 
		close(media_class->RTP_msg->RTP_fd);
	free(media_class->RTP_msg);
	
	if(media_class->media_buf != NULL)
		free(media_class->media_buf);
	free(media_class);
}

//客户端退出函数，释放内存
void 
client_realease(Client *client)
{
#ifdef DEBUG 
	printf("%s client who's IP is %s logout\n", client->name,client->IP);
	printf("-----------------------------------------------------------\n");
#endif
	
	list_del(&client->list);
	
	if(client->loading_media == H264 
		|| client->loading_media == MP4)
		media_class_realease(client->media_class[H264]);
	if(client->loading_media == AAC 
		|| client->loading_media == MP4)
		media_class_realease(client->media_class[AAC]);
		
	pthread_t RTSP_tid = client->RTSP_msg->RTSP_tid;
	if(client->RTSP_msg->RTSP_fd != -1) close(client->RTSP_msg->RTSP_fd);
	
	free(client->RTSP_msg->response_packet);
	free(client->RTSP_msg->income_packet);
	free(client->RTSP_msg);
	free(client);
	
	pthread_mutex_t mutex;
	pthread_mutex_lock(&mutex);
	server->client_num = server->client_num ? --(server->client_num) : 0;
	pthread_mutex_unlock(&mutex);
	
	pthread_cancel(RTSP_tid);	
}

//根据IP找到client，没找到新建client
bool 
client_locate(struct sockaddr_in *caddr)
{
	//判断incomeIP是否为新客户,是就创建新客户,创建RTSP的会话,
	//旧的就找出这个Client,作为入参传给handle函数
	Client *temp = NULL;
	struct list_head *pos;
	list_for_each(pos, &client_list) {	//在Client链表中里找对应的IP				
		temp = list_entry(pos, Client, list);
		if(!strcmp(temp->IP, inet_ntoa(caddr->sin_addr))) break;
	}
	//若指针到达回链表头代表找不到client，那应该是新client
	if(pos == &client_list)	return true;
	
	strcpy(temp->IP, inet_ntoa(caddr->sin_addr));
	temp->RTSP_msg->RTSP_port = caddr->sin_port;
	return false;
}

//客户端链表退出函数，释放内存
void 
client_realeaseAll(Client *client)
{
	Client *temp;
	struct list_head *pos, *next; 
	
	list_for_each_safe(pos, next, &client_list){
		temp = list_entry(pos, Client, list); 
		client_realease(temp);
	}
	server->client_num = 0;
}