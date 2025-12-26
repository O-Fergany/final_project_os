#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  
#include "config.h"
#include "sbuffer.h"


int storage_init();
int storage_insert(sensor_data_t *data);
void storage_free();
void storage_process_buffer(sbuffer_t *buffer);

#endif
