
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>

#include "Debug.h"
#include "Server_Client.h"
#include "SDP_setLines.h"
#include "RTSP_Session_tools.h"
#include "RTSP_Session.h"
#include "Network.h"
#include "RTP_transport.h"

void *
RTSP_Session_Process(void *arg)
{
	Client *client = arg;
	while(1) {
		int len = tcpRecv(client);
		if(len == -1) continue;
		
		//解析RTSP包，若出错暂时处理是返回
		parse_RTSP_packet(client, len);
		
	#ifdef DDEBUG
		printf("Receive %d bytes from %s\n", len, client->IP);
		printf("%s", client->RTSP_msg->income_packet);
		printf("\n-----------------------------------------------------------\n");
	#endif
		
		//准备工作做好后，将整个Client送进handle_rtsp_method中
		if(authenticationOK(client)) handle_rtsp_method[client->RTSP_msg->method](client);
		else continue;
		
		//发送packet
		len = tcpSend(client);
		
	#ifdef DDEBUG
		printf("Sent %d bytes to %s\n", len, client->IP);
		printf("%s", client->RTSP_msg->response_packet);
		printf("\n-----------------------------------------------------------\n");
	#endif

		//如果客户退出则释放相关内存，否则清除一些信息
		if(client->RTSP_msg->method == TEARDOWN) client_realease(client);
		else wash_client(client);
	}
}

void *
handle_OPTIONS(Client *client)
{	
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
		"RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
		client->RTSP_msg->CSeq, dateHeader(), supporte_methods);
}

void *
handle_DESCRIBE(Client *client)		//记录客户名字和播放的文件
{
	//client开始通过RTSP会话初始化参数
	client->state = Init; 
	
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	//获得用户名字
	char *strtmp = strstr(client->RTSP_msg->income_packet, "User-Agent:");
	if(strtmp != NULL) {
		strtmp = strtok(strtmp," ");
		strtmp = strtok(NULL," ");
		strcpy(client->name, strtmp);
	}
	
	//获得媒体文件名字
	strtmp = strtok(client->RTSP_msg->URI, "/");
	strtmp = strtok(NULL, "/");
	strtmp = strtok(NULL, "/");
	strcpy(client->filename, strtmp);
	
	//打开媒体文件
	if (!readFile(client)){
		snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
			"RTSP/1.0 404 File Not Found, Or In Incorrect Format\r\n"
			"CSeq: %s\r\n"
			"%s\r\n",
			client->RTSP_msg->CSeq,
			dateHeader());
				
		return 0;
	}
	
	//生成SDP描述
	char *sdpDescription = generate_SDP_description(client);
	if(sdpDescription == NULL) {
		handle_rtsp_method[BAD](client);
		return 0;
	}
	int sdpDescriptionSize = strlen(sdpDescription);
	
	//生成RTSP响应包
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
		"%s"
		"Content-Base: rtsp://%s:%u/%s/\r\n"
		"Content-Type: application/sdp\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s",
	    client->RTSP_msg->CSeq,
	    dateHeader(),
	    server->IP, server->RTSP_port, client->filename,
	    sdpDescriptionSize,
	    sdpDescription);
	
	free(sdpDescription);
}

void *
handle_SETUP(Client *client)	//记录服务器和客户的RTP/RTCP端口号，记录RTSP的会话ID
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	//要求client在Init状态才能进入
	if(client->state != Init) {
		handle_rtsp_method[BAD](client);
		return 0;
	}
	
	//仅支持RTP/AVP模式
	if(strstr(client->RTSP_msg->income_packet, "RTP/AVP") == NULL) {
		handle_rtsp_method[unsupportedTransport](client);
		return 0;
	}
	
	//随机生成会话ID，进行识别
	client->RTSP_msg->OurSessionId = our_random32();
	
	//记录客户的RTP/RTCP端口号
	int media_flag = get_udpport(client);
	if(media_flag == -1) {
		handle_rtsp_method[notFound](client);
		return 0;
	}
	
	//检查参数，将client状态改为Ready
	if(check_parameters(client)) client->state = Ready; 
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
		   "RTSP/1.0 200 OK\r\n"
		   "CSeq: %s\r\n"
		   "%s"
		   "Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
		   "Session: %08X\r\n\r\n",
		   client->RTSP_msg->CSeq,
		   dateHeader(),
		   client->IP, server->IP, 
		   client->media_class[media_flag]->RTP_msg->RTP_client_port, 
		   client->media_class[media_flag]->RTP_msg->RTCP_client_port, 
		   client->media_class[media_flag]->RTP_msg->RTP_server_port, 
		   client->media_class[media_flag]->RTP_msg->RTP_server_port + 1, 
		   client->RTSP_msg->OurSessionId);
}

void *
handle_PLAY(Client *client)		//记录客户的Timestamp，和RTP头的序列号的起始
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	//要求client在Ready状态才能进入
	if(client->state != Ready) {
		handle_rtsp_method[BAD](client);
		return 0;
	}
	
	//检验会话ID是否一致
	char *strtmp = strtok(client->RTSP_msg->income_packet,"\n");
	strtmp = strtok(NULL,"\n");
	strtmp = strtok(NULL,"\n");
	strtmp = strtok(NULL," ");
	strtmp = strtok(NULL,"\r");
	
	//初始化RTP信息
	char *rtp_info = get_rtp_info(client);

	//生成RTSP响应包
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
		"%s"
		"Session: %08X\r\n"
		"%s\r\n",
		client->RTSP_msg->CSeq,
		dateHeader(),
		client->RTSP_msg->OurSessionId,
		rtp_info);
		
	free(rtp_info);
	
	//创建线程开始RTP推流
	if(client->loading_media == H264 || client->loading_media == MP4) {
		if(client->media_class[H264]->RTP_msg->RTP_tid != 0) 
			pthread_cancel(client->media_class[H264]->RTP_msg->RTP_tid);
		
		if(pthread_create(&(client->media_class[H264]->RTP_msg->RTP_tid), NULL, H264_Entrance, client) != 0) {
			perror("pthread_create()");
			exit(1);
		}
	}
	if(client->loading_media == AAC  || client->loading_media == MP4) {
		if(client->media_class[AAC]->RTP_msg->RTP_tid != 0) 
			pthread_cancel(client->media_class[AAC]->RTP_msg->RTP_tid);
		
		if(pthread_create(&(client->media_class[AAC]->RTP_msg->RTP_tid), NULL, AAC_Entrance, client) != 0) {
			perror("pthread_create()");
			exit(1);
		}
	}
	client->state = Playing;
}

void *
handle_TEARDOWN(Client *client)
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
		"RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n",
		client->RTSP_msg->CSeq, dateHeader());
}

void *
handle_BAD(Client *client)
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
	   "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
	   dateHeader(), supporte_methods);
}

void *
handle_notSupported(Client *client)
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
	   "RTSP/1.0 405 Method Not Allowed\r\nCSeq: %s\r\n%sAllow: %s\r\n\r\n",
	   client->RTSP_msg->CSeq, dateHeader(), supporte_methods);
}

void *
handle_notFound(Client *client)
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
	   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
	   client->RTSP_msg->CSeq, dateHeader());
}

void *
handle_unsupportedTransport(Client *client)
{
	char *RTSP_rsp_packet = client->RTSP_msg->response_packet;
	
	snprintf(RTSP_rsp_packet, RTSP_BUFFER_SIZE,
	   "RTSP/1.0 461 Unsupported Transport\r\nCSeq: %s\r\n%s\r\n",
	   client->RTSP_msg->CSeq, dateHeader());
}
