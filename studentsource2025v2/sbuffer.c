#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "sbuffer.h"

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer==NULL) { fprintf(stderr,"we dont have memory for the sbuffer");return -1;}
    (*buffer)->head = NULL;
    (*buffer)->tail =NULL;
    pthread_mutex_init(&((*buffer)->lock),NULL);
    pthread_cond_init(&((*buffer)->readable), NULL);
    return 0;
}
int sbuffer_free(sbuffer_t **buffer) {
    if (buffer ==NULL || *buffer ==NULL)return -1;

    while ((*buffer) ->head) {
        sbuffer_node_t *curr = (*buffer)->head;
        (*buffer)->head=(*buffer)->head->next;
        free(curr);
    }
    free(*buffer);
    buffer =NULL;
    pthread_mutex_destroy(&(*buffer)->lock);
    pthread_cond_destroy(&(*buffer)->readable);
    return 0;
}
int sbuffer_add(sbuffer_t *buffer, sensor_data_t *data) {
    if (buffer==NULL) return-1;
    sbuffer_node_t *new_node = malloc(sizeof(sbuffer_node_t));
    if (new_node==NULL) return -1;
    new_node-> data = *data;
    new_node-> next = NULL;
    new_node-> read_count = 0;
    pthread_mutex_lock(&((buffer)->lock));
    if (buffer->tail ==NULL) {
        buffer->head = new_node;
        buffer->tail =new_node;
    }
    else {
        buffer->tail->next = new_node;
        buffer->tail = new_node;
    }
    pthread_cond_broadcast(&(buffer->readable));
    pthread_mutex_unlock(&((buffer)->lock));
    return 0;
}
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data, int reader_id) {
    if (buffer == NULL) return -1;
    pthread_mutex_lock(&(buffer->lock));

    while (buffer->head == NULL) {
        pthread_cond_wait(&(buffer->readable), &(buffer->lock));
    }

    sbuffer_node_t *node_to_read = buffer->head;

    *data = node_to_read->data;
    if (reader_id == 1 )access
    node_to_read->read_count++;

    if (node_to_read->read_count == 2) {
        buffer->head = node_to_read->next;
        if (buffer->head == NULL) {
            buffer->tail = NULL;
        }

        free(node_to_read);
    }
    pthread_mutex_unlock(&(buffer->lock));

    return 0;
}