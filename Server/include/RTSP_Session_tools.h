
#ifndef _RTSP_SESSION_TOOLS_H_
#define _RTSP_SESSION_TOOLS_H_

#include <stdbool.h>
#include "Server_Client.h"

//The little kid function
void parse_RTSP_packet(Client *client, int len);
bool check_parameters(Client *client);
unsigned our_random32();
char *dateHeader();
void wash_client(Client *client);
bool authenticationOK(Client *client);
bool readFile_internal ( media_class_t *media_class );
char *strrpc(char *str, char *oldstr, char *newstr );
void file2medianame ( Client *client );
bool readFile(Client *client);
void get_udpport_internal ( media_class_t *media_class, uint16_t RTP_cport, uint16_t RTCP_cport, char *IP);
int get_udpport (Client *client);
char *get_rtp_info(Client *client);

#endif
