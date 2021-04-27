
#ifndef _RTP_HEADER_H_
#define _RTP_HEADER_H_

#include "Server_Client.h"

typedef struct {
	
	unsigned char cc:4;
	unsigned char x:1;
	unsigned char p:1;
	unsigned char v:2;

	unsigned char pt:7;
	unsigned char m:1;

	unsigned short sequence_number;
	unsigned int timestamp;
	unsigned int ssrc;

} RTP_header;

void init_rtp_header(media_class_t *media_class);
void set_Timestamp(media_class_t *media_class);
void sequence_number_increase(media_class_t *media_class);
	
#endif