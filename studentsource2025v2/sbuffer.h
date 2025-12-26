#ifndef SBUFFER_H_
#define SBUFFER_H_

#include <pthread.h>
#include "config.h"


typedef struct sbuffer_node {
    sensor_data_t data;
    struct sbuffer_node *next;
    int access_check;
} sbuffer_node_t;


typedef struct sbuffer {
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_mutex_t lock;
    pthread_cond_t readable;
} sbuffer_t;


int sbuffer_init(sbuffer_t **buffer);

int sbuffer_free(sbuffer_t **buffer);

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);

int sbuffer_read_remove(sbuffer_t *buffer, sensor_data_t *data, int reader_id);

#endif
