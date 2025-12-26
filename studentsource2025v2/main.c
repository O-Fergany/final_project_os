#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"

static int pipe_fd[2];
static pid_t log_pid;

void* datamgr_thread_func(void* arg) {
    datamgr_process_buffer((sbuffer_t*)arg);
    return NULL;
}

void* storage_thread_func(void* arg) {
    storage_process_buffer((sbuffer_t*)arg);
    return NULL;
}

void run_log_process() {
    close(pipe_fd[1]);
    printf("[DEBUG] Log Process: Started and waiting for data...\n");
    FILE *log_fp = fopen("gateway.log", "w");
    if (!log_fp) exit(EXIT_FAILURE);
    char buffer[MSG_LENGTH];
    int sequence = 0;
    while (read(pipe_fd[0], buffer, MSG_LENGTH) > 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
        buffer[MSG_LENGTH-1] = '\0';
        fprintf(log_fp, "%d %s %s\n", sequence++, time_str, buffer);
        fflush(log_fp);
    }
    printf("[DEBUG] Log Process: EOF received. Closing log file and exiting.\n");
    fclose(log_fp);
    close(pipe_fd[0]);
    exit(EXIT_SUCCESS);
}

int write_to_log_process(char *msg) {
    char fixed_msg[MSG_LENGTH] = {0};
    strncpy(fixed_msg, msg, MSG_LENGTH - 1);
    if (write(pipe_fd[1], fixed_msg, MSG_LENGTH) == -1) return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) exit(EXIT_FAILURE);
    int port = atoi(argv[1]);
    int max_clients = atoi(argv[2]);
    sbuffer_t *shared_buffer = NULL;
    if (pipe(pipe_fd) == -1) exit(EXIT_FAILURE);
    log_pid = fork();
    if (log_pid == 0) run_log_process();
    close(pipe_fd[0]);
    write_to_log_process("Gateway system started.");
    sbuffer_init(&shared_buffer);
    FILE *fp_map = fopen("room_sensor.map", "r");
    if (fp_map) { datamgr_init(fp_map); fclose(fp_map); }
    pthread_t d_t, s_t;
    pthread_create(&d_t, NULL, datamgr_thread_func, shared_buffer);
    pthread_create(&s_t, NULL, storage_thread_func, shared_buffer);
    connmgr_listen(port, max_clients, shared_buffer);

    printf("[DEBUG] Main: Starting Joins...\n");

    pthread_join(d_t, NULL);
    printf("[DEBUG] Main: Datamgr joined.\n");

    pthread_join(s_t, NULL);
    printf("[DEBUG] Main: Storagemgr joined.\n");

    datamgr_free();
    printf("[DEBUG] Main: Datamgr_free done.\n");

    sbuffer_free(&shared_buffer);
    printf("[DEBUG] Main: Sbuffer_free done.\n");

    write_to_log_process("Gateway system shutting down.");
    close(pipe_fd[1]);
    printf("[DEBUG] Main: Pipe closed. Waiting for child %d...\n", log_pid);

    waitpid(log_pid, NULL, 0);
    printf("[DEBUG] Main: Reached end of process.\n");
    printf("[DEBUG] Main: Reached the very end of main function. Returning 0.\n");
    return 0;
}