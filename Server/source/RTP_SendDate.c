
#include <string.h>

#include "Debug.h"
#include "Server_Client.h"
#include "RTP_transport.h"
#include "RTP_header.h"
#include "Network.h"

void rtp_SendData(media_class_t *media_class, int len, int mark) 
{
	RTP_header *header = (RTP_header *)media_class->RTP_msg->RTP_buf;
	
	//调整RTP包的时间戳，序列号，以及是否最后一个包
	header->m = mark;	//对于视频，标记一帧的结束
	set_Timestamp(media_class);
	sequence_number_increase(media_class);

	//发送RTP包。
	int res = udpSendto(media_class, len);

#ifdef DDEBUG	
	printf("\nSend size [%d]: \t", res);
	for (int i = 0; i < 18; ++i) {
		if(i == 12) printf("| ");
		printf("%.2X ", media_class->RTP_msg->RTP_buf[i]);
	}
	printf("\n");
#endif	

	//memset RTP包内的playload部分，将load指针指回RTP PAYLOAD
	media_class->RTP_msg->load = media_class->RTP_msg->RTP_buf + sizeof(RTP_header);
	memset(media_class->RTP_msg->load + 2, 0, MTU - sizeof(RTP_header) - 2);

}
