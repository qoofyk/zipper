#include "conn_socket.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "utility.h"

#define CONN_SOCK_MAX_BUFLEN (MB(8))

static status_t error(char *msg) {
  perror(msg);
  return (-1);
}

int conn_socket_open(int port) {
  int listen_sockfd;
  struct sockaddr_in serv_addr;
  int n;

  listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sockfd < 0)
    return error("ERROR opening socket");

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);
  if (bind(listen_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    return error("ERROR on binding");
  listen(listen_sockfd, 5); // maximum of 5 pending connections

  PINF("Server start on port %d..., (sockfd=%d)", port, listen_sockfd);

  return listen_sockfd;
}

int conn_socket_accept(int listen_sockfd) {
  int conn_sockfd, clilen;
  struct sockaddr_in cli_addr;
  clilen = sizeof(cli_addr);
  conn_sockfd = accept(listen_sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (conn_sockfd < 0)
    return error("ERROR on accept");

  PINF("Server accepts connection (sockfd=%d)...", conn_sockfd);

  return conn_sockfd;
}

int conn_socket_connect(char *hostname, int port) {
  int sockfd, n;

  struct sockaddr_in serv_addr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return error("ERROR opening socket");
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(port);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    return error("ERROR connecting");
  PINF("Client connected to %s:%d, sockfd=(%d)", hostname, port, sockfd);

  return sockfd;
}

void conn_socket_close(int sockfd) {
  close(sockfd);
  PINF("Socket: close connection #(%d)", sockfd);
}

int conn_socket_recv(int sockfd, void *buff, size_t maxsize) {
  int nr_bytes = 0;
  int ret;
  while (nr_bytes < maxsize) {
    ret = recv(sockfd, buff + nr_bytes, maxsize - nr_bytes, 0);
    if (ret < 0)
      return ret;
    nr_bytes += ret;
    // if(ret = 0) return nr_bytes;
    PDBG("  %d bytes received", ret);
  }
  if (nr_bytes == maxsize) {
    return nr_bytes;
  } else {
    PERR("Lost package in message(%d/%lu) received", nr_bytes, maxsize);
    return -1;
  }
}

void conn_socket_send_ctrl(int sockfd, ctrl_msg_t ctrl_msg) {
  int n;
  n = conn_socket_send(sockfd, &ctrl_msg, sizeof(ctrl_msg));
  assert(n == sizeof(ctrl_msg));
  PDBG("sending CTRLMSG: type(%d), data msg size (%d bytes) \n", ctrl_msg.type,
       ctrl_msg.msg_size);
}

ctrl_msg_t conn_socket_recv_ctrl(int sockfd) {
  ctrl_msg_t ctrl_msg;
  int msg_size = sizeof(ctrl_msg);
  int n;
  n = conn_socket_recv(sockfd, &ctrl_msg, msg_size);
  assert(n == msg_size);

  PDBG("received CTRLMSG: type(%d), data msg size (%d bytes) \n", ctrl_msg.type,
       ctrl_msg.msg_size);
  return ctrl_msg;
}
