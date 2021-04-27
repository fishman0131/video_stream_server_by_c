
#include "Media_type.h"
#include "RTP_transport.h"

Media_msg_t media_msg[3] = {
	{"H.264", ".264", "track1", 96 , H264_Entrance},
	{"AAC Audio"  , ".aac", "track2", 96 , AAC_Entrance},
	{"MPEG4", ".mp4", "\0"    , 96 , NULL}
};