/*
 * Description:
 *
 * Author: Feng Li
 * e-mail: fengggli@yahoo.com
 */

#ifndef ELATIC_BROKER_H_
#define ELATIC_BROKER_H_
#include <string>

#ifdef __cplusplus
extern "C" {
  #include "hiredis.h"
#endif

#ifdef __cplusplus
}
typedef struct{
  char stream_name[80];
  redisContext *redis_context;
}broker_ctx;

broker_ctx mycontext;

static broker_ctx* broker_init(const char *field_name, const char* hostname, int port, int taskid,int groupid ){
    broker_ctx * context =  new broker_ctx();
    sprintf(context->stream_name, "%s%d",field_name, taskid); 

    redisContext *c;
    redisReply *reply;

    unsigned int isunix = 0;

    int queue_len = 128;
    int ii = 0;
      // taskid =1;

    PINF("running exp with server(%s:%d), taskid = %d",
       hostname, port, taskid);
    
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    if (isunix) {
      c = redisConnectUnixWithTimeout(hostname, timeout);
    } else {
      c = redisConnectWithTimeout(hostname, port, timeout);
    }
    if (c == NULL || c->err) {
      if (c) {
        printf("Connection error: %s\n", c->errstr);
        redisFree(c);
      } else {
        printf("Connection error: can't allocate redis context\n");
      }
      exit(1);
    }

    /* PING server */
    reply = (redisReply *)redisCommand(c, "auth 5ba4239a1a2b7cd8131da1e557f4264df7ef2083f8895eab1d30384f870a9d87");
    printf("auth: %s\n", reply->str);
    freeReplyObject(reply);

    /* PING server */
    reply = (redisReply *)redisCommand(c, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    context->redis_context= c;
}

static int broker_put(broker_ctx *context, int stepid, std::string values){
  std::string stream_name = context->stream_name;
  redisContext *c = context->redis_context;
  redisReply *reply;

  std::string commandString = "XADD ";
  commandString.append(stream_name);
  commandString.append(" MAXLEN ~ 1000 * ");
  commandString.append(" step ");
  commandString.append(std::to_string(stepid));

  // commandString.append(" field ")
  // commandString.append(fieldname)
  commandString.append(" valuelist ");

  commandString.append(values);

  // Info<< "----command is" << commandString << endl;

  reply = (redisReply *)redisCommand(c, commandString.c_str());
  if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
    PERR("Error on hiredis command ");
    if(reply)
      PERR("error info: %s", reply->str);
    return -1;
  }
  freeReplyObject(reply);
  return 0;
}

static void broker_finalize(broker_ctx * context){
          redisFree(context->redis_context);
};
#endif
#endif
