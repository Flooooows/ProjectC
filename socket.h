#ifndef _SOCKET_H_
#define _SOCKET_H_

int initSocketServer(int port);

int initSocketClient(char ServerIP[16], int Serverport);

#endif