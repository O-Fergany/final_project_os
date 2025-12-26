#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"
#include "connmgr.h"

static int active_sensor_threads = 0;
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;


void *handle_sensor_node(void *arg) {
    session_t *session = (session_t *)arg;
    sensor_data_t data;
    int result;
    int len;
    uint16_t current_id = 0;
    int first_data = 1; 

    while (1) 
    {
        len = sizeof(data.id);
        result = tcp_receive_timeout(session->socket, &data.id, &len, TIMEOUT);
        
        if (result != TCP_NO_ERROR || len <= 0) break; 

        if (first_data == 1) 
        {
            current_id = data.id;
            char msg[256];
            sprintf(msg, "Sensor node %hu has opened a new connection", current_id);
            write_to_log_process(msg);
            first_data = 0;
        }

        len = sizeof(data.value);
        result = tcp_receive_timeout(session->socket, &data.value, &len, TIMEOUT);
        if (result != TCP_NO_ERROR) break; 

        len = sizeof(data.ts);
        result = tcp_receive_timeout(session->socket, &data.ts, &len, TIMEOUT);
        if (result != TCP_NO_ERROR) break;

        sbuffer_insert(session->buffer, &data);
    }

    if (current_id != 0) 
    {
        char log_out[256];
        sprintf(log_out, "Sensor node %hu has closed the connection", current_id);
        write_to_log_process(log_out);
    }
    
    tcp_close(&session->socket);
    
    pthread_mutex_lock(&count_lock);
    active_sensor_threads--;
    pthread_mutex_unlock(&count_lock);

    free(session); 
    return NULL;
}


void connmgr_listen(int port, int max_conns, sbuffer_t *shared_buf) {
    tcpsock_t *server;
    int total_started_conns = 0;


    if (tcp_passive_open(&server, port) != TCP_NO_ERROR) {
        exit(EXIT_FAILURE);
    }

    while (total_started_conns < max_conns) 
    {
        tcpsock_t *client_sock = NULL;
        if (tcp_wait_for_connection(server, &client_sock) == TCP_NO_ERROR) 
        {
            session_t *session = malloc(sizeof(session_t));
            session->socket = client_sock;
            session->buffer = shared_buf;
            
            pthread_mutex_lock(&count_lock);
            active_sensor_threads++;
            total_started_conns++;
            pthread_mutex_unlock(&count_lock);
            
            pthread_t tid;
            pthread_create(&tid, NULL, handle_sensor_node, (void *)session);
            pthread_detach(tid);
        }
    }


    int check = 1;
    while (check) 
    {
        pthread_mutex_lock(&count_lock);
        if (active_sensor_threads <= 0) 
        {
            check = 0;
        }
        pthread_mutex_unlock(&count_lock);
        
        if (check) {
            usleep(1000); 
        }
    }

    sensor_data_t shutdown_mark;
    shutdown_mark.id = 0;
    shutdown_mark.value = 0;
    shutdown_mark.ts = 0;
    
    sbuffer_insert(shared_buf, &shutdown_mark);
    
    tcp_close(&server);
}
