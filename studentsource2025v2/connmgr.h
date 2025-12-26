#ifndef CONNMGR_H_
#define CONNMGR_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"

typedef struct {
    tcpsock_t *socket;
    sbuffer_t *buffer;
} session_t;

void connmgr_listen(int port, int max_conns, sbuffer_t *shared_buf);
void *handle_sensor_node(void *arg);
int write_to_log_process(char *msg);

#endif
