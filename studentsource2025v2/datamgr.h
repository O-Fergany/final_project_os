#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

typedef struct {
    sensor_id_t sensor_id;
    room_id_t room_id;
    double readings[RUN_AVG_LENGTH];
    int count;
    int insert_ptr;
    double running_avg;
    time_t last_modified;
} sensor_node_t;


void datamgr_init(FILE *fp_sensor_map);
void datamgr_process_buffer(sbuffer_t *buf);
void datamgr_free();
uint16_t datamgr_get_room_id(sensor_id_t id);
double datamgr_get_avg(sensor_id_t id);
time_t datamgr_get_last_modified(sensor_id_t id);

#endif
