
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Debug.h"
#include "Server_Client.h"
#include "RTP_transport.h"
#include "RTP_header.h"

#define AU_HEADER_SIZE 4

bool 
is_syncword(uint8_t *media_buf)		//判断同步字节
{
	if(media_buf[0] == 0xFF && (media_buf[1] & 0xF0) == 0xF0)
		return true;
	else return false;
}

int 
find_syncword(uint8_t *media_buf)	//寻找同步字节
{
	int i;
	for(i = 0; !is_syncword(&media_buf[i]); ++i);
	return i;
}

//获得音频帧的长度
uint16_t 
get_frame_len(uint8_t *media_buf, uint16_t adts_header_size)
{
	uint16_t frame_len;
	if(adts_header_size == 7) {
		frame_len = (uint16_t)((media_buf[3] & 0x03) << 11)
			  + (uint16_t)((media_buf[4] & 0xFF) << 3)
			  + (uint16_t)((media_buf[5] & 0xE0) >> 5)
			  - adts_header_size;
	}
	else if(adts_header_size == 9) {
		frame_len = (uint16_t)((media_buf[5] & 0x03) << 11)
			  + (uint16_t)((media_buf[6] & 0xFF) << 3)
			  + (uint16_t)((media_buf[7] & 0xE0) >> 5)
			  - adts_header_size;
	}
	return frame_len;
}

void *
AAC_Entrance(void *arg) 
{  
	Client *client = arg;
	
#ifdef DEBUG
	printf("Start %s stream to %s\n", client->media_class[AAC]->media_name, client->IP);
#endif

	uint8_t *media_buf = client->media_class[AAC]->media_buf;
	RTP_class_t *rtp_packet = client->media_class[AAC]->RTP_msg;

	init_rtp_header(client->media_class[AAC]);
	
	uint8_t *media_load = media_buf;	//记录文件流的读位置
	if(!is_syncword(media_buf))
		media_load += find_syncword(media_buf);
	uint16_t adts_header_size = (media_load[1] & 0x01) ? 7 : 9;
	
	while(media_load - media_buf < client->media_class[AAC]->media_size) {
		//同步字节判断
		if(!is_syncword(media_load)) 	
			media_load += find_syncword(media_load);
			
		//获得音频帧的长度
		uint16_t frame_len = get_frame_len(media_load, adts_header_size);
		
		//音频帧头
		rtp_packet->load[0] = 0x00;
		rtp_packet->load[1] = 0x10;
		rtp_packet->load[2] = (frame_len & 0x1fe0) >> 5;  
		rtp_packet->load[3] = (frame_len & 0x1f) << 3;  
		rtp_packet->load += AU_HEADER_SIZE;
		media_load += adts_header_size;
		
		//音频帧信息
		memcpy(rtp_packet->load, media_load, frame_len);
		rtp_packet->load += frame_len;
		media_load += frame_len;
		
		//更新RTP Header
		++rtp_packet->sequence_Basenumber;
		rtp_packet->Timestamp += client->media_class[AAC]->framesize;
		
		//发送音频帧
		rtp_SendData(client->media_class[AAC], (int)(rtp_packet->load-rtp_packet->RTP_buf), 1);
		
		//控制发送速率
		usleep((client->media_class[AAC]->framesize) *1000000 /(client->media_class[AAC]->sample_rate));
	}
}  
