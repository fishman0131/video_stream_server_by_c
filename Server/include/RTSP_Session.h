
#ifndef _RTSP_SESSION_H_
#define _RTSP_SESSION_H_

#include "Server_Client.h"

static char supporte_methods[100] = "OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN";

enum RSTP_method {
	//null,
	OPTIONS,
	DESCRIBE,
	SETUP,
	PLAY,
	TEARDOWN,
	BAD,
	notSupported,
	notFound,
	unsupportedTransport
};

void *RTSP_Session_Process(void *arg);
void *handle_OPTIONS(Client *client);
void *handle_DESCRIBE(Client *client);
void *handle_SETUP(Client *client);
void *handle_PLAY(Client *client);
void *handle_TEARDOWN(Client *client);
void *handle_BAD(Client *client);
void *handle_notSupported(Client *client);
void *handle_notFound(Client *client);
void *handle_unsupportedTransport(Client *client);

static void *(*handle_rtsp_method[])(Client *client) = {
	handle_OPTIONS,
	handle_DESCRIBE,
	handle_SETUP,
	handle_PLAY,
	handle_TEARDOWN,
	handle_BAD,
	handle_notSupported,
	handle_notFound,
	handle_unsupportedTransport
};

#endif
