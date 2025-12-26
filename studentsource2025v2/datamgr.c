#include "datamgr.h"
#include "lib/dplist.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sbuffer.h"
#include "config.h"

extern int write_to_log_process(char *msg);
static dplist_t *list = NULL;

static sensor_node_t *find_sensor(sensor_id_t id) {
    int i;
    for (i = 0; i < dpl_size(list); i++) {
        sensor_node_t *node = (sensor_node_t *)dpl_get_element_at_index(list, i);
        if (node->sensor_id == id) { 
            return node;
        }
    }
    return NULL;
}

void *node_copy(void *src) {
    sensor_node_t *dst = malloc(sizeof(sensor_node_t));
    if (dst == NULL) return NULL; 
    memcpy(dst, src, sizeof(sensor_node_t));
    return dst;
}

void node_free(void **element) {
    free(*element);
    *element = NULL;
}

int node_compare(void *x, void *y) {
    if (((sensor_node_t *)x)->sensor_id == ((sensor_node_t *)y)->sensor_id) return 0;
    return -1;
}

void datamgr_init(FILE *fp_sensor_map) {
    list = dpl_create(node_copy, node_free, node_compare);
    sensor_node_t temp;
    
    while (fscanf(fp_sensor_map, "%hu %hu", &temp.room_id, &temp.sensor_id) != EOF) {
        temp.count = 0;
        temp.insert_ptr = 0;
        temp.running_avg = 0;
        int j;
        for(j=0; j<RUN_AVG_LENGTH; j++) temp.readings[j] = 0;
        
        dpl_insert_at_index(list, &temp, 1000, true);
    }
}

void datamgr_process_buffer(sbuffer_t *buf) {
    sensor_data_t raw_data;
    char log_msg[200]; 

    while (sbuffer_read_remove(buf, &raw_data, 1) != -1) {
        sensor_node_t *node = find_sensor(raw_data.id);
        
        if (node == NULL) {
            sprintf(log_msg, "Received sensor data with invalid sensor node ID %hu", raw_data.id);
            write_to_log_process(log_msg);
        } else {
            //circular buffer for avg, watched some vids to be able to use it , not sure how to properly cite 
            node->readings[node->insert_ptr] = raw_data.value;
            node->insert_ptr++;
            if (node->insert_ptr >= RUN_AVG_LENGTH) {
                node->insert_ptr = 0;
            }

            if (node->count < RUN_AVG_LENGTH) {
                node->count++;
            }

            if (node->count == RUN_AVG_LENGTH) {
                double total = 0;
                int k;
                for (k = 0; k < RUN_AVG_LENGTH; k++) {
                    total = total + node->readings[k];
                }
                node->running_avg = total / RUN_AVG_LENGTH;

                if (node->running_avg > SET_MAX_TEMP) {
                    sprintf(log_msg, "Sensor node %hu reports it's too hot (avg temp = %f)", node->sensor_id, node->running_avg);
                    write_to_log_process(log_msg);
                }
                if (node->running_avg < SET_MIN_TEMP) {
                    sprintf(log_msg, "Sensor node %hu reports it's too cold (avg temp = %f)", node->sensor_id, node->running_avg);
                    write_to_log_process(log_msg);
                }
            }
        }
    }
}

void datamgr_free() {
    dpl_free(&list, true);
}
