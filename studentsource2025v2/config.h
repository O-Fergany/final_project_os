#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>

int write_to_log_process(char *msg);

typedef uint16_t sensor_id_t;
typedef uint16_t room_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

#define MSG_LENGTH 256
#define TIME_LENGTH 32

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 25.0
#endif

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 15.0
#endif

#ifndef TIMEOUT
#define TIMEOUT 10
#endif

#endif
