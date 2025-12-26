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
    for (int i = 0; i < dpl_size(list); i++) {
        sensor_node_t *node = (sensor_node_t *)dpl_get_element_at_index(list, i);
        if (node->sensor_id == id) return node;
    }
    return NULL;
}

void *node_copy(void *src) {
    sensor_node_t *dst = malloc(sizeof(sensor_node_t));
    memcpy(dst, src, sizeof(sensor_node_t));
    return dst;
}

void node_free(void **element) {
    free(*element);
    *element = NULL;
}

int node_compare(void *x, void *y) {
    return ((sensor_node_t *)x)->sensor_id - ((sensor_node_t *)y)->sensor_id;
}

void datamgr_init(FILE *fp_sensor_map) {
    list = dpl_create(node_copy, node_free, node_compare);
    sensor_node_t temp;
    while (fscanf(fp_sensor_map, "%hu %hu", &temp.room_id, &temp.sensor_id) == 2) {
        temp.count = 0;
        temp.insert_ptr = 0;
        temp.running_avg = 0;
        memset(temp.readings, 0, sizeof(double) * RUN_AVG_LENGTH);
        dpl_insert_at_index(list, &temp, 0, true);
    }
}

void datamgr_process_buffer(sbuffer_t *buf) {
    sensor_data_t raw_data;
    char log_msg[256];
    while (1) {
        if (sbuffer_read_remove(buf, &raw_data, 1) == -1) {
            printf("[DEBUG] Datamgr: Received shutdown mark. Thread exiting.\n");
            break;
        }

        sensor_node_t *node = find_sensor(raw_data.id);
        if (node == NULL) {
            snprintf(log_msg, 256, "Received sensor data with invalid sensor node ID %hu", raw_data.id);
            write_to_log_process(log_msg);
            continue;
        }

        node->readings[node->insert_ptr] = raw_data.value;
        node->insert_ptr = (node->insert_ptr + 1) % RUN_AVG_LENGTH;
        if (node->count < RUN_AVG_LENGTH) node->count++;

        if (node->count >= RUN_AVG_LENGTH) {
            double sum = 0;
            for (int i = 0; i < RUN_AVG_LENGTH; i++) sum += node->readings[i];
            node->running_avg = sum / RUN_AVG_LENGTH;

            if (node->running_avg > SET_MAX_TEMP) {
                snprintf(log_msg, 256, "Sensor node %hu reports it's too hot (avg temp = %f)", node->sensor_id, node->running_avg);
                write_to_log_process(log_msg);
            } else if (node->running_avg < SET_MIN_TEMP) {
                snprintf(log_msg, 256, "Sensor node %hu reports it's too cold (avg temp = %f)", node->sensor_id, node->running_avg);
                write_to_log_process(log_msg);
            }
        }
    }
}

void datamgr_free() {
    if (list) dpl_free(&list, true);
}