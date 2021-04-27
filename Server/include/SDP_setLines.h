
#ifndef _SDP_SETLINES_H_
#define _SDP_SETLINES_H_

#include "Server_Client.h"

unsigned get_framerate(const u_int8_t *sps);
void find_sps_pps(media_class_t *media_class, u_int8_t **sps, unsigned *spsSize, u_int8_t **pps, unsigned *ppsSize) ; 
void remove_Emulation_Bytes(u_int8_t *nal, unsigned *len);
char *set_H264_Lines(media_class_t *media_class) ;
unsigned get_sample_rate(uint8_t *media_buf);
char *set_AAC_Lines(media_class_t *media_class) ;
char *set_media_Lines(Client *client);
char *generate_SDP_description(Client *client);

#define media_bufsize 1000

#endif
