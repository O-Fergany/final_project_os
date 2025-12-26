#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sensor_db.h"
#include "config.h"
#include "sbuffer.h"

extern int write_to_log_process(char *msg);
static FILE *db_fp = NULL;

int storage_init() {
    db_fp = fopen("data.csv", "w");
    if (db_fp == NULL) return -1;
    write_to_log_process("A new data.csv file has been created.");
    return 0;
}

int storage_insert(sensor_data_t *data) {
    if (db_fp == NULL || data == NULL) return -1;
    if (fprintf(db_fp, "%hu, %f, %ld\n", data->id, data->value, (long)data->ts) > 0) {
        fflush(db_fp);
        char msg[256];
        snprintf(msg, 256, "Data insertion from sensor %hu succeeded.", data->id); 
        write_to_log_process(msg);
        return 0;
    }
    return -1;
}

void storage_free() {
    if (db_fp != NULL) {
        fclose(db_fp);
        db_fp = NULL;
        write_to_log_process("The data.csv file has been closed.");
    }
}

void storage_process_buffer(sbuffer_t *buffer) {
    sensor_data_t data;
    if (storage_init() != 0) return;

    while (1) {
        int result = sbuffer_read_remove(buffer, &data, 2);

        if (result == -1 || data.id == 0) {
            printf("[DEBUG] Storagemgr: Received shutdown mark. Exiting thread.\n");
            break;
        }
        storage_insert(&data);
    }
    storage_free();
}