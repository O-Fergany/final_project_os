#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "sbuffer.h"

int sbuffer_init(sbuffer_t **buffer) {
    sbuffer_t *temp = (sbuffer_t *)malloc(sizeof(sbuffer_t));
    if (temp == NULL) return -1;
    
    temp->head = NULL;
    temp->tail = NULL;
    
    pthread_mutex_init(&temp->lock, NULL);
    pthread_cond_init(&temp->readable, NULL);
    
    *buffer = temp;
    return 0;
}

int sbuffer_free(sbuffer_t **buffer) {
    if (buffer == NULL || *buffer == NULL) return -1;
    
    sbuffer_t *b = *buffer;
    sbuffer_node_t *current = b->head;
    
    while (current != NULL) {
        sbuffer_node_t *next_node = current->next;
        free(current);
        current = next_node;
    }
    
    pthread_mutex_destroy(&b->lock);
    pthread_cond_destroy(&b->readable);
    free(b);
    *buffer = NULL;
    return 0;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    if (buffer == NULL) return -1;
    
    sbuffer_node_t *new_elt = (sbuffer_node_t *)malloc(sizeof(sbuffer_node_t));
    if (new_elt == NULL) return -1;
    
    new_elt->data = *data;
    new_elt->next = NULL;
    new_elt->access_check = 0;

    pthread_mutex_lock(&buffer->lock);
    
    if (buffer->head == NULL) {
        buffer->head = new_elt;
        buffer->tail = new_elt;
    } else {
        buffer->tail->next = new_elt;
        buffer->tail = new_elt;
    }
    
    pthread_cond_broadcast(&buffer->readable);
    pthread_mutex_unlock(&buffer->lock);
    
    return 0;
}

int sbuffer_read_remove(sbuffer_t *buffer, sensor_data_t *data, int reader_id) {
    if (buffer == NULL) return -1;
    
    pthread_mutex_lock(&buffer->lock);

    sbuffer_node_t *iterator = NULL;
    int found_node = 0;
    
    while (found_node == 0) {
        iterator = buffer->head;
        while (iterator != NULL) {
            if ((iterator->access_check & reader_id) == 0) {
                found_node = 1;
                break;
            }
            iterator = iterator->next;
        }
        
        if (found_node == 0) {
            pthread_cond_wait(&buffer->readable, &buffer->lock);
        }
    }

    *data = iterator->data;
    iterator->access_check = iterator->access_check | reader_id;
    
    int is_done = 0;
    if (data->id == 0) {
        is_done = 1;
    }

    pthread_cond_broadcast(&buffer->readable);

    while (buffer->head != NULL) {
        if (buffer->head->access_check == 3) {
            sbuffer_node_t *temp_del = buffer->head;
            buffer->head = buffer->head->next;
            
            if (buffer->head == NULL) {
                buffer->tail = NULL;
            }
            
            free(temp_del);
        } else {
            break; 
        }
    }
    
    pthread_mutex_unlock(&buffer->lock);
    
    if (is_done == 1) {
        return -1;
    }
    return 0;
}
