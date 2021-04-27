
#include <stdint.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "Server_Client.h"
#include "RTP_transport.h"
#include "RTP_header.h"

void init_rtp_header(media_class_t *media_class) {
	
	RTP_header *header = (RTP_header *)media_class->RTP_msg->RTP_buf;
	
	header->v = 2;
	header->p = 0;
	header->x = 0;
	header->cc = 0;
	header->m = 0;
	header->pt = media_msg[media_class->media_type].playoad_type;
	header->sequence_number = media_class->RTP_msg->sequence_Basenumber;
	header->timestamp = media_class->RTP_msg->Timestamp;
	header->ssrc = media_class->RTP_msg->ssrc;
	
	media_class->RTP_msg->load = media_class->RTP_msg->RTP_buf + sizeof(RTP_header);
	
}

void sequence_number_increase(media_class_t *media_class){
	
	RTP_header *header = (RTP_header *)media_class->RTP_msg->RTP_buf;
	
	unsigned short sequence = ntohs(header->sequence_number);
	sequence++;
	header->sequence_number = htons(sequence);
}

void set_Timestamp(media_class_t *media_class){
/*	
	// Begin by converting from "struct timeval" units to RTP timestamp units:
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32_t timestampIncrement = (90000 * tv.tv_sec);
	timestampIncrement += (u_int32_t)((2.0 * 90000*tv.tv_usec + 1000000.0)/2000000);
*/	

	RTP_header *header = (RTP_header *)media_class->RTP_msg->RTP_buf;

	unsigned int timestamp = ntohl(header->timestamp);
	timestamp = media_class->RTP_msg->Timestamp;
	header->timestamp = htonl(timestamp);
}
