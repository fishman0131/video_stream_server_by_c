
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "SDP_setLines.h"
#include "AVC.h"
#include "Server_Client.h"
#include "Base64.h"
#include "Media_type.h"

unsigned 
get_framerate(const u_int8_t *sps) 
{
	unsigned framerate;
	unsigned num_units_in_tick;
	unsigned time_scale;
	
	if(sps[14] & 0x01) {
		num_units_in_tick = (unsigned)(sps[15] << 24) 
					+ (unsigned)(sps[16] << 16) 
					+ (unsigned)(sps[17] << 8) 
					+ (unsigned) sps[18];
		time_scale = (unsigned)(sps[19] << 24) 
					+(unsigned)(sps[20] << 16) 
					+(unsigned)(sps[21] << 8) 
					+(unsigned) sps[22];
	}
	framerate = (time_scale/num_units_in_tick)/2;
	
	return framerate;
}

void 
find_sps_pps(media_class_t *media_class, u_int8_t **sps, unsigned *spsSize, u_int8_t **pps, unsigned *ppsSize) 
{
	const uint8_t *nal, *next_nal;
	uint8_t *end;
	bool sps_done, pps_done;
	
	sps_done = false;
	pps_done = false;
	end = media_class->media_buf + media_class->media_size;
	nal = ff_avc_find_startcode(media_class->media_buf, end);
	
	while (!(sps_done && pps_done) && nal < end) {
		while (!*(nal++));  	// skip current startcode
		
		next_nal = ff_avc_find_startcode(nal, end);	// find next startcode
		uint8_t nal_type = nal[0] & 0x1F;
		
		if ( nal_type == 7 && sps_done == false ) {	// SPS NALU
			*spsSize = (unsigned)(next_nal - nal);
			*sps = calloc(sizeof(u_int8_t), *spsSize); 
			memcpy(*sps, nal, *spsSize);
			sps_done = true;
		}
		
		if ( nal_type == 8 && pps_done == false ) {	// PPS NALU
			*ppsSize = (unsigned)(next_nal - nal);
			*pps = calloc(sizeof(u_int8_t), *ppsSize); 
			memcpy(*pps, nal, *ppsSize);
			pps_done = true;
		}
		nal = next_nal;
	}
}

void 
remove_Emulation_Bytes(u_int8_t *nal, unsigned *len)
{
	u_int8_t *nal_temp = calloc(sizeof(u_int8_t), *len);
	memcpy(nal_temp, nal, *len);
	
	unsigned j = 0;
	for(unsigned i = 0; i < *len; ++i) {
		if( i+2 < *len    && 
		    0 == nal[i]   &&
		    0 == nal[i+1] &&
		    3 == nal[i+2]) {
			nal[j++] = nal_temp[i++];
			nal[j++] = nal_temp[i++];
		}
		else nal[j++] = nal_temp[i];
	}
	
	*len = j;
	free(nal_temp);
}

char *
set_H264_Lines(media_class_t *media_class)
{
	char *H264_Line = calloc(sizeof(char), media_bufsize);
	
	u_int8_t* sps; unsigned spsSize;
	u_int8_t* pps; unsigned ppsSize;
	
	find_sps_pps(media_class, &sps, &spsSize, &pps, &ppsSize);
	
	remove_Emulation_Bytes(pps, &ppsSize);
	remove_Emulation_Bytes(sps, &spsSize);
	
	if(spsSize > 22) media_class->framerate = get_framerate(sps);
	
	u_int32_t profile_level_id;
	// sanity check
	if (spsSize < 4) profile_level_id = 0;
	// profile_idc|constraint_setN_flag|level_idc
	else profile_level_id = (sps[1]<<16)|(sps[2]<<8)|sps[3];
	
	char sps_base64[spsSize];
	char pps_base64[ppsSize];
	base64_encode(sps, sps_base64, spsSize);
	base64_encode(pps, pps_base64, ppsSize);
	
	snprintf(H264_Line, media_bufsize,
		"m=video 0 RTP/AVP %d\r\n"
		"c=IN IP4 0.0.0.0\r\n"
		"b=AS:500\r\n"
		"a=rtpmap:%d H264/90000\r\n"
		"a=fmtp:%d packetization-mode=1"
		";profile-level-id=%06X"
		";sprop-parameter-sets=%s,%s\r\n"
		"a=control:%s\r\n",
		media_msg[H264].playoad_type,
		media_msg[H264].playoad_type,
		media_msg[H264].playoad_type,
		profile_level_id,
		sps_base64, pps_base64,
		media_msg[H264].track
	);
	
	free(sps);
	free(pps);
	
	return H264_Line;
}