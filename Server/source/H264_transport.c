
#include <string.h>
#include <unistd.h>

#include "Debug.h"
#include "Server_Client.h"
#include "RTP_transport.h"
#include "RTP_header.h"
#include "AVC.h"

// 解析NALU，并组装到RTP包的playload部分, 然后rtp_SendData
void H264_LoadNAL(media_class_t *media_class, const uint8_t *nal, int size, int last)
{
	int mark;	//(M)ark bit
	uint8_t type = nal[0] & 0x1F;
	if(type >= 1 && type <= 5) {
		media_class->RTP_msg->Timestamp += (90000/(media_class->framerate));
		mark = 1;
		// control transmission speed
		usleep(1000000/(media_class->framerate));
	}
	else mark = 0;
	
	RTP_class_t *rtp_playload = media_class->RTP_msg;
	
	//根据NAL大小，选择分组方式
	if (size <= (MTU - sizeof(RTP_header))){
		//聚合分组
		if(rtp_playload->aggregation){
			uint8_t curNRI = (uint8_t)(nal[0] & 0x60);           // NAL NRI
			int buffered_size = (int)(rtp_playload->load - rtp_playload->RTP_buf - sizeof(RTP_header));
			
			//判断RTP剩余空间是否能装下这个NAL
			if (buffered_size + 2 + size > MTU) {
				rtp_SendData(media_class, (int)(rtp_playload->load-rtp_playload->RTP_buf), mark);
				buffered_size = 0;
			}
			
			if (buffered_size == 0){
				*rtp_playload->load++ = (uint8_t)(24 | curNRI);  // STAP-A = 24
			} else {
				uint8_t lastNRI = (uint8_t)(rtp_playload->RTP_buf[12] & 0x60);	//NRI取所有NALU的NRI最大值
				if (curNRI > lastNRI){  // if curNRI > lastNRI, use new curNRI
					rtp_playload->RTP_buf[12] = (uint8_t)((rtp_playload->RTP_buf[12] & 0x9F) | curNRI);
				}
			}
			
			//所有聚合NALU的F只要有一个为1则设为1
			rtp_playload->RTP_buf[12] |= (nal[0] & 0x80); 
			
			// NALU Size + NALU Header + NALU Data
			// Load16(rtp_playload->load, (uint16_t)size);
			uint16_t size_ = ntohs((uint16_t) size);
			memcpy(rtp_playload->load, &size_, 2);	// NAL size
			rtp_playload->load += 2;
			
			memcpy(rtp_playload->load, nal, size);	// NALU Header & Data
			rtp_playload->load += size;
			
			// meet last NAL, send all buf
			if (last == 1){
				rtp_SendData(media_class, (int)(rtp_playload->load-rtp_playload->RTP_buf), mark);
			}
		}
		//单NALU分组
		else {
			memcpy((rtp_playload->RTP_buf) + sizeof(RTP_header), nal, size);
			rtp_SendData(media_class, size + sizeof(RTP_header), mark);
		}
	}
	//分片分组
	else {
		if(rtp_playload->load > rtp_playload->RTP_buf + 12){
			rtp_SendData(media_class, (int)(rtp_playload->load-rtp_playload->RTP_buf), 0);
		}
		
		int headerSize;
		uint8_t *buff = media_class->RTP_msg->RTP_buf + sizeof(RTP_header);;
		//uint8_t type = nal[0] & 0x1F;
		uint8_t nri  = nal[0] & 0x60;
		
		//FU Indicator
		buff[0] = 28;   	// FU Indicator; FU-A Type = 28
		buff[0] |= nri;

		//FU Header
		buff[1] = type;     // FU Header uses NALU Header
		buff[1] |= 1 << 7;  // S(tart) = 1
		headerSize = 2;
		size -= 1;
		nal += 1;
		
		while(size + headerSize + 12 > MTU){
			memcpy(&buff[headerSize], nal, (size_t)(MTU - headerSize - 12));
			rtp_SendData(media_class, MTU, 0);
			nal += (MTU - headerSize - 12);
			size -= (MTU - headerSize - 12);
			buff[1] &= ~(1 << 7);  // buff[1] & 0111111, S(tart) = 0
		}
		buff[1] |= 1 << 6;      // buff[1] | 01000000, E(nd) = 1
		memcpy(&buff[headerSize], nal, size);
		rtp_SendData(media_class, size + headerSize + sizeof(RTP_header), mark);
	}
}

void *H264_Entrance(void *arg) 
{	
	Client *client = arg;
	
#ifdef DEBUG
	printf("Start %s stream to %s\n", client->media_class[H264]->media_name, client->IP);
#endif

	init_rtp_header(client->media_class[H264]);
	
	const uint8_t *nal, *next_nal;
	uint8_t *end = client->media_class[H264]->media_buf + client->media_class[H264]->media_size;
	nal = ff_avc_find_startcode(client->media_class[H264]->media_buf, end);
		
	while (nal < end){
		while (!*(nal++));  // skip current startcode
		
		next_nal = ff_avc_find_startcode(nal, end);  // find next startcode
		
		// send a NALU (except NALU startcode)
		H264_LoadNAL(client->media_class[H264], nal, (int)(next_nal-nal), next_nal==end);
			
		nal = next_nal;
	}
}