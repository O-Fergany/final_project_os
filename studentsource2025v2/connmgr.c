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
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

void *handle_sensor_node(void *arg) {
    session_t *session = (session_t *)arg;
    sensor_data_t data;
    int result, len;
    uint16_t current_id = 0;

    while (1) {
        len = sizeof(data.id);
        result = tcp_receive_timeout(session->socket, &data.id, &len, TIMEOUT);
        if (result != TCP_NO_ERROR) {
            printf("[DEBUG] Connmgr: tcp_receive (ID) failed with code %d for sensor %hu\n", result, current_id);
            break;
        }

        if (current_id == 0) {
            current_id = data.id;
            char msg[256];
            snprintf(msg, 256, "Sensor node %hu has opened a new connection", current_id);
            write_to_log_process(msg);
        }

        len = sizeof(data.value);
        result = tcp_receive_timeout(session->socket, &data.value, &len, TIMEOUT);
        if (result != TCP_NO_ERROR) {
            printf("[DEBUG] Connmgr: tcp_receive (Value) failed for sensor %hu\n", current_id);
            break;
        }

        len = sizeof(data.ts);
        result = tcp_receive_timeout(session->socket, &data.ts, &len, TIMEOUT);
        if (result != TCP_NO_ERROR) {
            printf("[DEBUG] Connmgr: tcp_receive (TS) failed for sensor %hu\n", current_id);
            break;
        }

        sbuffer_insert(session->buffer, &data);
    }

    printf("[DEBUG] Connmgr: Thread for sensor %hu is EXITING the loop.\n", current_id);

    if (current_id != 0) {
        char log_out[256];
        snprintf(log_out, 256, "Sensor node %hu has closed the connection", current_id);
        write_to_log_process(log_out);
    }

    tcp_close(&session->socket);
    free(session);

    pthread_mutex_lock(&count_lock);
    active_sensor_threads--;
    printf("[DEBUG] Connmgr: Decoupled thread. Active threads now: %d\n", active_sensor_threads);
    pthread_mutex_unlock(&count_lock);
    return NULL;
}

    printf("[DEBUG] Connmgr: Thread for sensor %hu exiting.\n", current_id);

    tcp_close(&session->socket);
    free(session);

    pthread_mutex_lock(&count_lock);
    active_sensor_threads--;
    pthread_mutex_unlock(&count_lock);
    return NULL;
}

void connmgr_listen(int port, int max_conns, sbuffer_t *shared_buf) {
    tcpsock_t *server;
    if (tcp_passive_open(&server, port) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    int total_started_conns = 0;
    while (total_started_conns < max_conns) {
        tcpsock_t *client_sock;
        printf("[DEBUG] Connmgr: Waiting for connection %d/%d...\n", total_started_conns + 1, max_conns);
        if (tcp_wait_for_connection(server, &client_sock) == TCP_NO_ERROR) {
            printf("[DEBUG] Connmgr: Connection %d accepted.\n", total_started_conns + 1);
            session_t *session = malloc(sizeof(session_t));
            session->socket = client_sock;
            session->buffer = shared_buf;

            pthread_mutex_lock(&count_lock);
            active_sensor_threads++;
            total_started_conns++;
            pthread_mutex_unlock(&count_lock);

            pthread_t tid;
            pthread_create(&tid, NULL, handle_sensor_node, session);
            pthread_detach(tid);
        }
    }

    int last_reported_count = -1;
    while (1) {
        pthread_mutex_lock(&count_lock);
        if (active_sensor_threads != last_reported_count) {
            printf("[DEBUG] Connmgr: Active threads remaining: %d\n", active_sensor_threads);
            last_reported_count = active_sensor_threads;
        }
        if (active_sensor_threads <= 0) {
            pthread_mutex_unlock(&count_lock);
            break;
        }
        pthread_mutex_unlock(&count_lock);
        usleep(100000);
    }

    printf("[DEBUG] Connmgr: All sensors disconnected. Inserting 2 shutdown marks.\n");
    usleep(500000);
    sensor_data_t shutdown_mark = {0, 0, 0};
    sbuffer_insert(shared_buf, &shutdown_mark);
    sbuffer_insert(shared_buf, &shutdown_mark);
    tcp_close(&server);
}