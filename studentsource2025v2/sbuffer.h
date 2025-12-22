#ifndef _SENSOR_BUFFER_H_
#define _SENSOR_BUFFER_H_

#include <pthread.h>
#include "config.h"


typedef struct sbuffer_node {
    sensor_data_t data;
    struct sbuffer_node *next;
    int read_count;
}
sbuffer_node_t;
typedef struct sbuffer {
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_mutex_t lock;
    pthread_cond_t readable;
}sbuffer_t;

int sbuffer_init(sbuffer_t **buffer);
int sbuffer_init(sbufferr_t **buffer);
int sbuffer_add(sbuffer_t *buffer, sensor_data_t *data);
int sbufferr_remove(sbuffer_t *buffer, sensor_data_t *data);


#endif