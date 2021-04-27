
#ifndef _MEDIA_TYPE_H_
#define _MEDIA_TYPE_H_

#define SMN 2 		//支持媒体格式的数目

enum Media { 	//支持媒体格式, MP4 = H264 + AAC
	H264, 
	AAC, 
	MP4 
};	

typedef struct {
	
	char name[10];
	char suffix[10];
	char track[10];
	unsigned playoad_type;
	
	void *(*rtp_Entrance)(void *arg);
	
} Media_msg_t;

extern Media_msg_t media_msg[3];

#endif