#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "sbuffer.h"

static sbuffer_node_t *find_node(sbuffer_t *buffer, int reader_id) {
    sbuffer_node_t *curr = buffer->head;
    while (curr != NULL) {
        if (!(curr->access_check & reader_id)) return curr;
        curr = curr->next;
    }
    return NULL;
}

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return -1;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    pthread_mutex_init(&((*buffer)->lock), NULL);
    pthread_cond_init(&((*buffer)->readable), NULL);
    return 0;
}

int sbuffer_free(sbuffer_t **buffer) {
    if (buffer == NULL || *buffer == NULL) return -1;
    pthread_mutex_lock(&((*buffer)->lock));
    sbuffer_node_t *curr = (*buffer)->head;
    while (curr) {
        sbuffer_node_t *temp = curr;
        curr = curr->next;
        free(temp);
    }
    pthread_mutex_unlock(&((*buffer)->lock));
    pthread_mutex_destroy(&((*buffer)->lock));
    pthread_cond_destroy(&((*buffer)->readable));
    free(*buffer);
    *buffer = NULL;
    return 0;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    if (buffer == NULL) return -1;
    sbuffer_node_t *new_node = malloc(sizeof(sbuffer_node_t));
    if (new_node == NULL) return -1;
    new_node->data = *data;
    new_node->next = NULL;
    new_node->access_check = 0;
    pthread_mutex_lock(&(buffer->lock));
    if (buffer->tail == NULL) {
        buffer->head = new_node;
        buffer->tail = new_node;
    } else {
        buffer->tail->next = new_node;
        buffer->tail = new_node;
    }
    pthread_cond_broadcast(&(buffer->readable));
    pthread_mutex_unlock(&(buffer->lock));
    return 0;
}

int sbuffer_read_remove(sbuffer_t *buffer, sensor_data_t *data, int reader_id) {
    if (buffer == NULL) return -1;
    pthread_mutex_lock(&(buffer->lock));

    sbuffer_node_t *node = NULL;
    while ((node = find_node(buffer, reader_id)) == NULL) {
        pthread_cond_wait(&(buffer->readable), &(buffer->lock));
    }

    *data = node->data;
    node->access_check |= reader_id;
    int is_end = (data->id == 0);

    if (is_end) printf("[DEBUG] Sbuffer: Reader %d RECEIVED SHUTDOWN.\n", reader_id);
    else printf("[DEBUG] Sbuffer: Reader %d read ID %hu, New Mask: %d\n", reader_id, data->id, node->access_check);

    pthread_cond_broadcast(&(buffer->readable));

    while (buffer->head != NULL && buffer->head->access_check == 3) {
        sbuffer_node_t *temp = buffer->head;
        printf("[DEBUG] Sbuffer: Fully read node ID %hu being DELETED.\n", temp->data.id);
        buffer->head = buffer->head->next;
        if (buffer->head == NULL) buffer->tail = NULL;
        free(temp);
        pthread_cond_broadcast(&(buffer->readable));
    }
    
    pthread_mutex_unlock(&(buffer->lock));
    return is_end ? -1 : 0;
}
