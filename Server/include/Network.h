
#ifndef _NETWORK_H
#define _NETWORK_H

#include "Server_Client.h"

/*   TCP    */
bool tcpInit();
int tcpSend(Client *client);
int tcpRecv(Client *client);

/*   UDP    */
bool udpInit(media_class_t *media_class, char *IP);
int udpSendto(media_class_t *media_class, int len);
//int udpRecvfrom(Client *client);

#endif
