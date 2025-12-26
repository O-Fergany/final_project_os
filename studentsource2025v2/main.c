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

void watchdog_handler(int sig) {
    if (log_pid > 0) kill(log_pid, SIGKILL);
    exit(1);
}

void* datamgr_thread_func(void* arg) {
    sbuffer_t* b = (sbuffer_t*)arg;
    datamgr_process_buffer(b);
    return NULL;
}

void* storage_thread_func(void* arg) {
    sbuffer_t* b = (sbuffer_t*)arg;
    storage_process_buffer(b);
    return NULL;
}

void run_log_process() {
    close(pipe_fd[1]);
    FILE *log_fp = fopen("gateway.log", "w");
    if (log_fp == NULL) exit(1);

    char buffer[MSG_LENGTH];
    int seq = 0;

    while (read(pipe_fd[0], buffer, MSG_LENGTH) > 0) {
        time_t raw_t;
        struct tm *info;
        char time_buffer[80];

        time(&raw_t);
        info = localtime(&raw_t);
        strftime(time_buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        
        fprintf(log_fp, "%d %s %s\n", seq, time_buffer, buffer);
        seq++;
        fflush(log_fp);
    }

    fclose(log_fp);
    close(pipe_fd[0]);
    exit(0);
}

int write_to_log_process(char *msg) {
    pthread_mutex_lock(&pipe_lock);
    char buf[MSG_LENGTH];
    int i;
    for(i = 0; i < MSG_LENGTH; i++) buf[i] = 0;
    strncpy(buf, msg, MSG_LENGTH - 1);
    
    int bytes = write(pipe_fd[1], buf, MSG_LENGTH);
    pthread_mutex_unlock(&pipe_lock);
    
    if (bytes != MSG_LENGTH) return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    
    signal(SIGALRM, watchdog_handler);
    alarm(10); 

    int p = atoi(argv[1]);
    int mc = atoi(argv[2]);
    sbuffer_t *sb;

    if (pipe(pipe_fd) == -1) exit(1);
    
    log_pid = fork();
    if (log_pid == -1) exit(1);
    
    if (log_pid == 0) {
        run_log_process();
    } else {
        close(pipe_fd[0]);
        
        write_to_log_process("Gateway system started.");
        
        sbuffer_init(&sb);
        FILE *f_map = fopen("room_sensor.map", "r");
        if (f_map != NULL) { 
            datamgr_init(f_map);
            fclose(f_map); 
        }

        pthread_t t1, t2;
        pthread_create(&t1, NULL, datamgr_thread_func, (void*)sb);
        pthread_create(&t2, NULL, storage_thread_func, (void*)sb);

        connmgr_listen(p, mc, sb);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        
        datamgr_free();
        sbuffer_free(&sb);
        
        write_to_log_process("Gateway system shutting down.");
        
        close(pipe_fd[1]);
        waitpid(log_pid, NULL, 0);
        
        alarm(0);
    }
    return 0;
}
