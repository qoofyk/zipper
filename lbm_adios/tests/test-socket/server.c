/** http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html */
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include "logging.h"
#include "unistd.h"
#include "utils/conn_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* After an client is up, server will send fixed-size message to client*/

static char *help_str = "Usage: %s [-p port] \n";

int main(int argc, char *argv[]) {
  int listen_sockfd, n, opt;
  int data_sockfd, ctrl_sockfd;
  int i;
  ctrl_msg_t ctrl_msg;

  int port = 2008;
  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    default: /* '?' */
      fprintf(stderr, help_str, argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  char *msg_buffer;
  int msg_size;

  listen_sockfd = conn_socket_open(port);
  if (listen_sockfd < 0)
    exit(-1);

  ctrl_sockfd = conn_socket_accept(listen_sockfd);
  data_sockfd = conn_socket_accept(listen_sockfd);

  ctrl_msg = conn_socket_recv_ctrl(ctrl_sockfd);
  if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
  }

  if (ctrl_msg.type == MSG_META) {
    msg_size = ctrl_msg.msg_size;
  } else {
    PERR("Expected to receive msg_type= 3");
  }
  printf("Message from client: msg_size(%d)\n", msg_size);
  msg_buffer = malloc(msg_size);
  memset(msg_buffer, 0, msg_size);

  ctrl_msg.type = MSG_BUFFER_PREPARED;
  conn_socket_send_ctrl(ctrl_sockfd, ctrl_msg);

  while (1) {
    n = conn_socket_recv(data_sockfd, msg_buffer, msg_size);
    if (n < 0) {
      perror("ERROR reading socket");
      exit(1);
    }
    // printf("(%d bytes are read)\n", n);

    ctrl_msg = conn_socket_recv_ctrl(ctrl_sockfd);
    if (ctrl_msg.type == MSG_EXIT)
      break;
  }

  conn_socket_close(data_sockfd);
  conn_socket_close(ctrl_sockfd);
  conn_socket_close(listen_sockfd);

  return 0;
}
