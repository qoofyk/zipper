#include <sys/socket.h> // send/receive)
/*TODO: 1. let server accepts multiple connections. (e.g.
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/)*/

/** Server api, support only one client*/
typedef enum {
  MSG_OK = 0,
  MSG_DATA = 1, // regular data msg
  MSG_EXIT = 2, // send by client to notify the end
  MSG_META =
      3, // send by client to notify the maximum data message it will send
  MSG_BUFFER_PREPARED = 4, // server has allocated receive buffer
} msg_type_t;

typedef struct {
  msg_type_t type;
  int msg_size; // DATA msg size
} ctrl_msg_t;

int conn_socket_open(int port);
int conn_socket_accept(int listen_sockfd);

/** Client api*/
int conn_socket_connect(char *server_addr, int port);

/** Common api*/
/* See https://stackoverflow.com/a/2862176/6261848*/
void conn_socket_close(int sockfd);

/* Data channel*/
static int conn_socket_send(int sockfd, void *buff, size_t size) {
  return send(sockfd, buff, size, 0);
}
int conn_socket_recv(int sockfd, void *buff, size_t maxsize);

/** Contrl channel*/

/** send msg type, -1 if failed*/
void conn_socket_send_ctrl(int sockfd, ctrl_msg_t t);

/** return msg type, -1 if failed*/
ctrl_msg_t conn_socket_recv_ctrl(int sockfd);
