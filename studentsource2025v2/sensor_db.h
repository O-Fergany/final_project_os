#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // Required for C11 bool type
#include "config.h"
#include "sbuffer.h"

/**
 * Requirement 6: Initialize the storage manager.
 * Opens (or creates) the CSV file in append mode.
 */
int storage_init();

/**
 * Requirement 6: Insert a sensor measurement into the CSV file.
 * Format: id, value, timestamp
 */
int storage_insert(sensor_data_t *data);

/**
 * Closes the storage manager and ensures all data is saved.
 */
void storage_free();

/**
 * Thread function to process the shared buffer for the storage manager.
 * Uses Reader ID 2 to identify as the Storage Manager.
 */
void storage_process_buffer(sbuffer_t *buffer);

#endif /* _SENSOR_DB_H_ */