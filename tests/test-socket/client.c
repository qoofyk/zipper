/** http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"
#include "utils/conn_socket.h"
#include <sys/socket.h> // send/receive)
#include <unistd.h>

static char *help_str =
    "Usage: %s [-p port] [-s message_size] [-n iterations] hostname\n";

int main(int argc, char *argv[]) {
  int data_sockfd, ctrl_sockfd;
  int n, opt, i;

  int port = 2008;
  int msg_size = KB(64);
  int iterations = 10000;
  ctrl_msg_t ctrl_msg;

  char *buffer;

  while ((opt = getopt(argc, argv, "p:s:n:")) != -1) {
    fprintf(stderr, "passing option %c\n", opt);
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 's':
      msg_size = atoi(optarg);
      break;

    case 'n':
      iterations = atoi(optarg);
      break;
    default: /* '?' */
      fprintf(stderr, help_str, argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    exit(EXIT_FAILURE);
  }
  printf("optind=%d\n", optind);
  char *hostname = argv[optind];
  PINF("running exp with server(%s:%d), with msg size(%d), iterations(%d)",
       hostname, port, msg_size, iterations);

  ctrl_sockfd = conn_socket_connect(hostname, port);
  data_sockfd = conn_socket_connect(hostname, port);
  if (ctrl_sockfd < 0)
    perror("ERROR opening ctrl socket");
  if (data_sockfd < 0)
    perror("ERROR opening data socket");

  ctrl_msg.type = MSG_META;
  ctrl_msg.msg_size = msg_size;

  conn_socket_send_ctrl(ctrl_sockfd, ctrl_msg);

  buffer = malloc(msg_size);
  bzero(buffer, msg_size);

  ctrl_msg = conn_socket_recv_ctrl(ctrl_sockfd);
  assert(ctrl_msg.type == MSG_BUFFER_PREPARED);

  double t_start = get_cur_time();
  for (i = 0; i < iterations; i++) {

    n = conn_socket_send(data_sockfd, buffer, msg_size);
    if (n < 0) {
      perror("ERROR writing to socket");
      exit(1);
    }

    if (i == iterations - 1)
      ctrl_msg.type = MSG_EXIT;
    else
      ctrl_msg.type = MSG_OK;
    conn_socket_send_ctrl(ctrl_sockfd, ctrl_msg);
  }
  double t_elapsed = get_cur_time() - t_start;
  double size_in_MB = (iterations / 1000000.0) * msg_size;
  PINF("Bandwidth %.5f MB/s(t_elapsed = %.3f s, transfered = %.3fMB)",
       size_in_MB / t_elapsed, t_elapsed, size_in_MB);

  conn_socket_close(data_sockfd);
  conn_socket_close(ctrl_sockfd);

  free(buffer);
  return 0;
}
