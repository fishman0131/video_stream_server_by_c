
#ifndef _RTP_TRANSPORT_H_
#define _RTP_TRANSPORT_H_

#include "Server_Client.h"

void rtp_SendData(media_class_t *media_class, int len, int mark);

void H264_LoadNAL(media_class_t *media_class, const uint8_t *nal, int size, int last);
void *H264_Entrance(void *arg);

bool is_syncword(uint8_t *media_buf);
int find_syncword(uint8_t *media_buf);
uint16_t get_frame_len(uint8_t *media_buf, uint16_t adts_header_size);
void *AAC_Entrance(void *arg);
	
#endif