#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"

/* * The number of previous measurements used to calculate the moving average.
 */
#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

/*
 * Structure to hold sensor state.
 * Using a circular buffer (readings + insert_ptr) is more efficient than shifting arrays.
 */
typedef struct {
    sensor_id_t sensor_id;
    room_id_t room_id;
    double readings[RUN_AVG_LENGTH];
    int count;          // Current number of readings (up to RUN_AVG_LENGTH)
    int insert_ptr;     // Index for the next incoming measurement
    double running_avg;
    time_t last_modified;
} sensor_node_t;

/*
 * Simple macro for fatal error reporting
 */
#define DATAMGR_ASSERT(condition, msg) \
    do { if (!(condition)) { perror(msg); exit(EXIT_FAILURE); } } while(0)

/**
 * Initializes the internal sensor list based on the provided map file.
 * \param fp_sensor_map File pointer to the room-to-sensor mapping file.
 */
void datamgr_init(FILE *fp_sensor_map);

/**
 * Reads data from the shared sbuffer and updates the internal sensor list.
 * Stops when the buffer is empty or a terminator (ID=0) is encountered.
 * \param buf Pointer to the shared sbuffer.
 */
void datamgr_process_buffer(sbuffer_t *buf);

/**
 * Cleans up all allocated memory for the sensor list.
 */
void datamgr_free();

/**
 * Returns the room ID for a specific sensor.
 * \param id The sensor ID to look up.
 * \return The room ID associated with the sensor.
 */
uint16_t datamgr_get_room_id(sensor_id_t id);

/**
 * Returns the most recent running average for a specific sensor.
 * \param id The sensor ID to look up.
 * \return The current moving average temperature.
 */
double datamgr_get_avg(sensor_id_t id);

/**
 * Returns the timestamp of the last reading received for a sensor.
 * \param id The sensor ID to look up.
 * \return The time of the last update.
 */
time_t datamgr_get_last_modified(sensor_id_t id);

#endif /* DATAMGR_H_ */
