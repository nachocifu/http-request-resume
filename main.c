#include "../ProxyFilter/proxyFilter.h"
#include "../common.h"
#include "parser.h"
#include "socks.h"
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for close


/**
*Name: clientToProxy
*
*Description:
*
*@param char * port
*@return int validate
**/
int clientToProxy(char *port) {
  puts("PING::init clienttoproxy");

  in_port_t servPort = atoi(port); // First arg:  local port

  int servSock;
  if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("socket() failed\n");
    return -1;
  }

  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
    printf("bind() failed\n");
    return -1;
  }

  if (listen(servSock, MAXPENDING) < 0) {
    printf("listen() failed\n");
    return -1;
  }
  puts("PING::pre for en clienttoproxy");
  for (;;) {
    struct sockaddr_in clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);

    int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (clntSock < 0) {
      printf("accept() failed\n");
      return -1;
    }

    puts("PING::post accept en for en clienttoproxy");
    char clntName[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
                  sizeof(clntName)) != NULL)
      printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
    else
      puts("Unable to get client address\n");

    HandleTCPClient(clntSock);
  }
}

/**
*Name: HAndleTCPClient
*
*Description:
*
*@param int clntSocket
*@return void
**/
void HandleTCPClient(int clntSocket) {
  char buffer[BUFSIZE];
  memset(buffer, 0, BUFSIZE - 1);
  ssize_t n = 1;
  struct user_request *user_request;
  struct user_response *user_response;

  // handle socks comunication
  struct TCP_connection *tcp_link = socks_handler(clntSocket);
  if (tcp_link == NULL)
    return; // unable to establish connection between client and target
  else      // return target welcome msg
    send(clntSocket, tcp_link->msg, tcp_link->msgLength, 0);

  // init dialog between client and target
  while (n > 0) {

    // receive msg from client to target
    n = recv(clntSocket, buffer, BUFSIZE - 1, 0);
    if (n < 0) {
      printf("recv() failed\n");
      return;
    }
    // generate request struct
    user_request = parse(buffer);
    user_request->tcp_link = tcp_link;

    // pass the request to filter module
    user_response = filterResponse(user_request);

    // return msg from target to client
    n = send(clntSocket, user_response->msg, user_response->msgLength, 0);
    if (n < 0) {
      printf("send() failed\n");
      return;
    }
  }

  close(clntSocket);
}