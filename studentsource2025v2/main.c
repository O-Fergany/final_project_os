#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"

static int pipe_fd[2];
static pid_t log_pid;
static pthread_mutex_t pipe_lock = PTHREAD_MUTEX_INITIALIZER;

// Signal handler to force exit if threads deadlock
void watchdog_handler(int sig) {
    printf("\n[WATCHDOG] 10 second limit reached. Forcing shutdown to prevent VM hang.\n");
    if (log_pid > 0) kill(log_pid, SIGKILL);
    exit(EXIT_FAILURE);
}

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
    FILE *log_fp = fopen("gateway.log", "w"); // [cite: 81, 83]
    if (!log_fp) exit(EXIT_FAILURE);

    char buffer[MSG_LENGTH];
    int sequence = 0;

    while (read(pipe_fd[0], buffer, MSG_LENGTH) > 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[26];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t); // 
        
        buffer[MSG_LENGTH-1] = '\0';
        fprintf(log_fp, "%d %s %s\n", sequence++, time_str, buffer); // 
        fflush(log_fp);
    }

    fclose(log_fp);
    close(pipe_fd[0]);
    exit(EXIT_SUCCESS);
}

int write_to_log_process(char *msg) {
    pthread_mutex_lock(&pipe_lock); // [cite: 79]
    char fixed_msg[MSG_LENGTH] = {0};
    strncpy(fixed_msg, msg, MSG_LENGTH - 1);
    ssize_t bytes_written = write(pipe_fd[1], fixed_msg, MSG_LENGTH); // [cite: 77]
    pthread_mutex_unlock(&pipe_lock);
    return (bytes_written == MSG_LENGTH) ? 0 : -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) exit(EXIT_FAILURE);
    
    // Set up the 10-second watchdog
    signal(SIGALRM, watchdog_handler);
    alarm(10); 

    int port = atoi(argv[1]); // [cite: 61]
    int max_clients = atoi(argv[2]); // [cite: 61]
    sbuffer_t *shared_buffer = NULL;

    if (pipe(pipe_fd) == -1) exit(EXIT_FAILURE); // [cite: 77]
    log_pid = fork(); // [cite: 53, 55]
    if (log_pid == 0) run_log_process();
    close(pipe_fd[0]);
    
    write_to_log_process("Gateway system started.");
    
    sbuffer_init(&shared_buffer); // [cite: 57]
    FILE *fp_map = fopen("room_sensor.map", "r"); // [cite: 71]
    if (fp_map) { 
        datamgr_init(fp_map); // [cite: 70]
        fclose(fp_map); 
    }

    pthread_t d_t, s_t;
    pthread_create(&d_t, NULL, datamgr_thread_func, shared_buffer); // [cite: 56]
    pthread_create(&s_t, NULL, storage_thread_func, shared_buffer); // [cite: 56]

    connmgr_listen(port, max_clients, shared_buffer); // [cite: 60]

    pthread_join(d_t, NULL);
    pthread_join(s_t, NULL);
    
    datamgr_free();
    sbuffer_free(&shared_buffer);
    
    write_to_log_process("Gateway system shutting down.");
    
    close(pipe_fd[1]);
    waitpid(log_pid, NULL, 0); // 
    
    // Cancel alarm if finished successfully
    alarm(0); 
    return 0;
}
