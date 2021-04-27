
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "SDP_setLines.h"
#include "AVC.h"
#include "Server_Client.h"
#include "Base64.h"
#include "Media_type.h"

#define sdp_size 5000
#define media_bufsize 1000

char *
set_media_Lines(Client *client)
{
	char *media_Line = calloc(sizeof(char), SMN * media_bufsize);
	if(media_Line == NULL) return NULL;
	char *media_Sonline[SMN];
	
	switch(client->loading_media) {
		case MP4: 
			media_Sonline[H264] = set_H264_Lines(client->media_class[H264]);
			media_Sonline[AAC] = set_AAC_Lines(client->media_class[AAC]);
			snprintf(media_Line, SMN * media_bufsize,
				"%s%s",
				media_Sonline[H264],
				media_Sonline[AAC]);
			free(media_Sonline[H264]);
			free(media_Sonline[AAC]);
			break;
			
		case H264: 
			media_Sonline[H264] = set_H264_Lines(client->media_class[H264]);
			strcpy(media_Line, media_Sonline[H264]);
			free(media_Sonline[H264]);
			break;
			
		case AAC:
			media_Sonline[AAC] = set_AAC_Lines(client->media_class[AAC]);
			strcpy(media_Line, media_Sonline[AAC]);
			free(media_Sonline[AAC]);
			break;
	}

	return media_Line;
}

char *
generate_SDP_description(Client *client)
{
	char *sdpLines = calloc(sizeof(char), sdp_size);
	char *media_line = set_media_Lines(client);
	
	snprintf(sdpLines, sdp_size,
		"v=0\r\n"
		"o=- %ld%06ld %d IN IP4 %s\r\n"
		"s=%s, streamed by the %s\r\n"
		"i=%s\r\n"
		"t=0 0\r\n"
		"a=tool:%s\r\n"
		"a=type:broadcast\r\n"
		"a=control:*\r\n"
		"a=range:npt=0-\r\n"
		"a=x-qt-text-nam:%s, streamed by the %s\r\n"
		"a=x-qt-text-inf:%s\r\n"
		"%s",
		client->creation_time.tv_sec, client->creation_time.tv_usec, 1, server->IP,
		media_msg[client->loading_media].name, server->name,
		client->filename,
		server->name,
		media_msg[client->loading_media].name, server->name,
		client->filename,
		media_line);
	
	free(media_line);

	return sdpLines;
}