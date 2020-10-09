#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#include "socket.h"
#include "utils.h"

int initSocketServer(int port){
	int sockfd;
	struct sockaddr_in addr;	
	int ret;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	checkNeg(sockfd,"socket server creation error");
	
	/* no socket error */
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(sockfd,(struct sockaddr *)&addr,sizeof(addr));
	checkNeg(ret,"server bind error");
	ret = listen(sockfd,5);
	checkNeg(ret,"server listen error");
	return sockfd;
}

int initSocketClient(char ServerIP[16], int Serverport){
	int sockfd;
  	int ret;
  	struct sockaddr_in addr;
  	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	checkNeg(sockfd,"socket client creation error");

	memset(&addr,0,sizeof(addr)); /* en System V */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Serverport);
	addr.sin_addr.s_addr = inet_addr(ServerIP);
	ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
	checkNeg(ret,"connect client error");
	
    return sockfd;
}
