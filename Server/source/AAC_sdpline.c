
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "SDP_setLines.h"
#include "AVC.h"
#include "Server_Client.h"
#include "Base64.h"
#include "Media_type.h"

unsigned get_sample_rate(uint8_t *media_buf)
{
	unsigned sample_rate;
	switch((media_buf[2] & 0x3C) >> 2){
		case 0x0: sample_rate = 96000; break;
		case 0x1: sample_rate = 88200; break;
		case 0x2: sample_rate = 64000; break;
		case 0x3: sample_rate = 48000; break;
		case 0x4: sample_rate = 44100; break;
		case 0x5: sample_rate = 32000; break;
		case 0x6: sample_rate = 24000; break;
		case 0x7: sample_rate = 22050; break;
		case 0x8: sample_rate = 16000; break;
		case 0x9: sample_rate = 12000; break;
		case 0xA: sample_rate = 11025; break;
		case 0xB: sample_rate = 8000 ; break;
		case 0xC: sample_rate = 7350 ; break;
		default:  return 0;
	}
	
	return sample_rate;
}

char *
set_AAC_Lines(media_class_t *media_class) 
{
	char *AAC_Line = calloc(sizeof(char), media_bufsize);
	if(AAC_Line == NULL) return NULL;
	
	media_class->sample_rate = get_sample_rate(media_class->media_buf);
	
	snprintf(AAC_Line, media_bufsize,
		"m=audio 0 RTP/AVP %d\r\n"
		"c=IN IP4 0.0.0.0\r\n"
		"b=AS:96\r\n"
		"a=rtpmap:%d MPEG4-GENERIC/%u/2\r\n"
		"a=fmtp:%d streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210\r\n"
		"a=control:%s\r\n",
		media_msg[AAC].playoad_type,
		media_msg[AAC].playoad_type,
		media_class->sample_rate,
		media_msg[AAC].playoad_type,
		media_msg[AAC].track
		);
	
	return AAC_Line;
}