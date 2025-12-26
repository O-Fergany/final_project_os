#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sensor_db.h"
#include "config.h"
#include "sbuffer.h"

extern int write_to_log_process(char *msg);
static FILE *db_fp = NULL;

int storage_init() {
    db_fp = fopen("data.csv", "a");
    if (db_fp == NULL) return -1;
    write_to_log_process("Connection to SQL server established.");
    return 0;
}

int storage_insert(sensor_data_t *data) {
    if (db_fp == NULL || data == NULL) return -1;
    fprintf(db_fp, "%hu, %f, %ld\n", data->id, data->value, (long)data->ts);
    fflush(db_fp);
    return 0;
}

void storage_free() {
    if (db_fp != NULL) {
        fclose(db_fp);
        db_fp = NULL;
        write_to_log_process("Connection to SQL server lost.");
    }
}

void storage_process_buffer(sbuffer_t *buffer) {
    sensor_data_t data;
    if (storage_init() != 0) return; // Open once

    while (1) {
        int result = sbuffer_read_remove(buffer, &data, 2);

        // Process data BEFORE checking for exit if possible,
        // but ID 0 is strictly a shutdown signal.
        if (data.id == 0 || result == -1) {
            // Around line 40 (inside the if (data.id == 0) block)
            printf("[DEBUG] Storagemgr: Received shutdown mark. Exiting thread.\n");
            break;
        }
        storage_insert(&data);
    }
    storage_free(); // This closes the file and flushes data
}