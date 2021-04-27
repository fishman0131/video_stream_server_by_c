
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "Server_Client.h"
#include "RTSP_Session.h"
#include "RTSP_Session_tools.h" 
#include "Network.h"

//解析包,并将关键信息放进client结构体
void 
parse_RTSP_packet(Client *client, int len)
{
	//recv返回值为0,可能TCP连接中断
	if(len == 0) 	client_realease(client);

	char income_packet[RTSP_BUFFER_SIZE]; 
	strcpy(income_packet, client->RTSP_msg->income_packet);
	char *URI = client->RTSP_msg->URI;
	char *version = client->RTSP_msg->version;
	char *CSep = client->RTSP_msg->CSeq;
	char *temp;
	
	//parse method
	if(client->RTSP_msg->method == -1){ 
		if (strstr(income_packet, "OPTIONS"			) != NULL) 
			client->RTSP_msg->method = OPTIONS;
		
		else if (strstr(income_packet, "DESCRIBE"	) != NULL) 
			client->RTSP_msg->method = DESCRIBE;
		
		else if (strstr(income_packet, "SETUP"		) != NULL) 
			client->RTSP_msg->method = SETUP;
		
		else if (strstr(income_packet, "PLAY"		) != NULL) 
			client->RTSP_msg->method = PLAY;
		
		else if (strstr(income_packet, "TEARDOWN"	) != NULL) 
			client->RTSP_msg->method = TEARDOWN;
		
		else client->RTSP_msg->method = notSupported; //格式不对应,或者不支持
	}
	
	//parse URI
	temp = strstr(income_packet, "rtsp://");
	if ( !strcmp(URI, "\0") && temp != NULL ) {
		temp = strtok(temp, " ");
		if(temp == NULL) {
		#ifdef DEBUG
			printf("Warning: get URI failed\n");
		#endif
			return;
		}
		strcpy(URI, temp);
	}
	
	//parse version
	if(!strcmp(version, "\0")) {
		strcpy(income_packet, client->RTSP_msg->income_packet);
		temp = strstr(income_packet, "RTSP/1.0");
		if(temp != NULL) strcpy(version, "RTSP/1.0");
		else {
			client->RTSP_msg->method = BAD;
			return;
		}
	}
	
	//parse CSep
	strcpy(income_packet, client->RTSP_msg->income_packet);
	temp = strstr(income_packet, "CSeq:");
	if(!strcmp(CSep, "\0") && temp != NULL) {
		temp = strtok(temp, " ");
		temp = strtok(NULL, "\r");
		if(temp != NULL) strcpy(CSep, temp);
		else {
		#ifdef DEBUG
			printf("Warning: get CSep failed\n");
		#endif
			return;
		}
	}
}

void 
wash_client(Client *client)
{
	client->RTSP_msg->method = -1;
	strcpy(client->RTSP_msg->CSeq, "\0");
	strcpy(client->RTSP_msg->URI , "\0");
	
	memset(client->RTSP_msg->income_packet, 0, RTSP_BUFFER_SIZE);
	memset(client->RTSP_msg->response_packet, 0, RTSP_BUFFER_SIZE);
}

bool 
authenticationOK(Client *client)
{
	if(client->RTSP_msg->method == -1) return false;
	if(!strcmp(client->RTSP_msg->CSeq, "\0")) return false;
	if(!strcmp(client->RTSP_msg->URI , "\0")) return false;
	
	return true;
}

char *
dateHeader()
{
	static char timebuf[100];
	time_t tt = time(NULL);
	strftime(timebuf, sizeof timebuf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
	
	return timebuf;
}

unsigned 
our_random32()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned rand = ((unsigned)tv.tv_sec * 1103515245 + random()%(unsigned)tv.tv_usec) & 0xffffffff;
	
	return rand;
}

//在发RTP包前,检查参数是否正确和齐全
bool 
check_parameters(Client *client)
{
	switch(client->loading_media) {
		case MP4:
			if(!(client->media_class[H264]->init_done 
			  && client->media_class[AAC]->init_done)) 
				return false;
			break;
			
		case H264: 
			if(!client->media_class[H264]->init_done) return false;
			break;
			
		case AAC:
			if(!client->media_class[AAC]->init_done) return false;
			break;
	}

	return true;
}

char *
strrpc(char *str, char *oldstr, char *newstr)
{
    char bstr[strlen(str)];	//转换缓冲区
    memset(bstr,0,sizeof(bstr));
 
    for ( int i = 0 ; i < strlen(str) ; i++){
        if ( !strncmp(str+i,oldstr,strlen(oldstr))){	//查找目标字符串
            strcat(bstr,newstr);
            i += strlen(oldstr) - 1;
        }else{
        	strncat(bstr , str + i , 1);	//保存一字节进缓冲区
	    }
    }
 
    strcpy(str,bstr);
    return str;
}

void 
file2medianame ( Client *client )
{
	char *media_name = client->media_class[client->loading_media]->media_name;
	if(client->loading_media == AAC || client->loading_media == H264) {
		strcpy(media_name, client->filename);
		return;
	}
	char strtmp[50];
	for ( int i = 0; i < 2; ++i ) { 
		media_name = client->media_class[i]->media_name;
		strcpy(strtmp, client->filename);
		strrpc(strtmp, media_msg[MP4].suffix, media_msg[i].suffix);
		strcpy(media_name, strtmp);
	}
}

bool 
readFile_internal ( media_class_t *media_class )
{
	FILE *fp = fopen(media_class->media_name, "r");
	if(fp == NULL) return false;
	
	struct stat info;
	stat(media_class->media_name, &info);
	media_class->media_size = info.st_size;
	
	media_class->media_buf = (uint8_t *) calloc(sizeof(uint8_t), media_class->media_size);
	if(media_class->media_buf == NULL) return false;
	
	if (fread(media_class->media_buf, 1, media_class->media_size, fp) 
		!= media_class->media_size) 	
		return false;
		
	fclose(fp);
	
	return true;
}

bool 
readFile(Client *client)
{
	//判断请求的文件格式,暂时先看文件后缀
	if ( strstr(client->filename, media_msg[H264].suffix) != NULL ) {
		
		client->loading_media = H264;
		if ( client->media_class[H264] == NULL) //按照loading_media,初始化media类
			client->media_class[H264] = media_class_init(H264);
			
		file2medianame(client);		//将URI中的请求文件名转换成媒体文件名
		if ( !readFile_internal(client->media_class[H264]) )	//读文件流
			return false;
	}
	else if ( strstr(client->filename, media_msg[AAC].suffix) != NULL ) {
		
		client->loading_media = AAC;
		if ( client->media_class[AAC] == NULL ) 
			client->media_class[AAC] = media_class_init(AAC);
		
		file2medianame(client);
		if ( !readFile_internal(client->media_class[AAC]) )
			return false;
	}
	else if ( strstr(client->filename, media_msg[MP4].suffix ) != NULL ) {
		
		client->loading_media = MP4;
		if ( client->media_class[H264] == NULL )
			client->media_class[H264] = media_class_init(H264);
		
		if ( client->media_class[AAC] == NULL )
			client->media_class[AAC] = media_class_init(AAC);
		
		file2medianame(client);
		if ( !(readFile_internal(client->media_class[AAC])
			&& readFile_internal(client->media_class[H264])) )
			return false;
	}
	else 
		return false;

	return true;
}

void 
get_udpport_internal ( media_class_t *media_class, uint16_t RTP_cport, 
	uint16_t RTCP_cport, char *IP)
{
	//随机生成资源号给client
	media_class->RTP_msg->ssrc = our_random32();
	media_class->RTP_msg->RTP_client_port = RTP_cport;
	media_class->RTP_msg->RTCP_client_port = RTCP_cport;
	//为之后的RTP/RTCP分别新建一个端口
	
	if(!udpInit(media_class, IP)) return;
	
	media_class->init_done = true;
}

int 
get_udpport ( Client *client )
{
	//记录客户的RTP/RTCP端口号
	uint16_t RTP_cport, RTCP_cport;
	char *strcmp = strstr(client->RTSP_msg->income_packet, "client_port=");
	if(strcmp != NULL) {
		strcmp =strtok(strcmp, "=");
		strcmp = strtok(NULL,"-");					
		RTP_cport = atoi(strcmp);
		strcmp = strtok(NULL,"\r");
		RTCP_cport = atoi(strcmp);
	#ifdef DEBUG 
		if(RTP_cport != RTCP_cport - 1) 
			printf("Warning: RTP_client_port != RTCP_client_port-1\n");
	#endif
	}
	else return -1;
	
	switch(client->loading_media) {
		case MP4:
			if(!client->media_class[H264]->init_done) {
				get_udpport_internal(client->media_class[H264], RTP_cport, RTCP_cport, client->IP);
				if(client->media_class[H264]->init_done) return H264;
			}
			else if(!client->media_class[AAC]->init_done) {
				get_udpport_internal(client->media_class[AAC], RTP_cport, RTCP_cport, client->IP);
				if(client->media_class[AAC]->init_done) return AAC;
			}
			break;
			
		case H264: 
			if(!client->media_class[H264]->init_done) {
				get_udpport_internal(client->media_class[H264], RTP_cport, RTCP_cport, client->IP);
				if(client->media_class[H264]->init_done) return H264;
			}
			break;
			
		case AAC:
			if(!client->media_class[AAC]->init_done) {
				get_udpport_internal(client->media_class[AAC], RTP_cport, RTCP_cport, client->IP);
				if(client->media_class[AAC]->init_done) return AAC;
			}
			break;
	}
	return -1;
}

char *
get_rtp_info_internal ( Client *client, media_class_t *media_class )
{
	char *rtp_info_son = calloc(sizeof(char), 400);
	
	//随机生成序列号
	media_class->RTP_msg->sequence_Basenumber = (unsigned short)our_random32();
	//时间戳怎么来？时间戳初始化为非负随机数，然后按照帧率计算时间戳的增加
	media_class->RTP_msg->Timestamp = our_random32();
	
	sprintf(rtp_info_son,    
		"url=rtsp://%s:%d/%s/%s;"
		"seq=%d;"
		"rtptime=%u\r\n",
		server->IP, RTSP_Port, 
		media_class->media_name, media_msg[media_class->media_type].track,
		media_class->RTP_msg->sequence_Basenumber,
		media_class->RTP_msg->Timestamp);
		
	return rtp_info_son;
}

char *
get_rtp_info(Client *client)
{
	char *rtp_info = calloc(sizeof(char), 1000);
	char *rtp_info_son[SMN];
	
	switch(client->loading_media) {
		case MP4:
			rtp_info_son[H264] = get_rtp_info_internal(client, client->media_class[H264]);
			rtp_info_son[AAC]  = get_rtp_info_internal(client, client->media_class[AAC]);
		
			snprintf(rtp_info, 1000,
				"RTP-Info: "
				"%s,%s",
				rtp_info_son[H264],
				rtp_info_son[AAC]);
				
			free(rtp_info_son[H264]);
			free(rtp_info_son[AAC]);
			break;
			
		case H264: 
			rtp_info_son[H264] = get_rtp_info_internal(client, client->media_class[H264]);
			
			snprintf(rtp_info, 1000,
				"RTP-Info: "
				"%s",
				rtp_info_son[H264]);
				
			free(rtp_info_son[H264]);
			break;
			
		case AAC:
			rtp_info_son[AAC] = get_rtp_info_internal(client, client->media_class[AAC]);
			
			snprintf(rtp_info, 1000,
				"RTP-Info: "
				"%s",
				rtp_info_son[AAC]);
				
			free(rtp_info_son[AAC]);
			break;
	}
	
	return rtp_info;
}
