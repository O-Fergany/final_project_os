#ifndef CONNMGR_H_
#define CONNMGR_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"

// Define the struct HERE so it's visible to all functions below
typedef struct {
    tcpsock_t *socket;
    sbuffer_t *buffer;
} session_t;

/**
 * Starts the connection manager and listens for incoming sensor connections.
 */
void connmgr_listen(int port, int max_conns, sbuffer_t *shared_buf);

/**
 * Internal thread function to handle an individual sensor session.
 */
void *handle_sensor_node(void *arg);

// Prototype for logging (implemented in main.c)
int write_to_log_process(char *msg);

#endif