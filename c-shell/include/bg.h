#ifndef BG_H
#define BG_H
#include <sys/types.h>

typedef struct
{
    pid_t pid;
    char command[64];
    int done;
    int status;
} Process;

typedef struct
{
    int job_n;
    pid_t pgid;
    pid_t reported_pid;
    Process processes[100];
    int process_count;
    int state; // 0: running, 1: done, -1: stopped
} Job;

int add(
    pid_t pgid,
    pid_t reported_pid,
    Process *processes,
    int count
);

void sig_handler(int sig);
int activities(void);
void stop(int job_number);
void sighup();
int spdJobs();

#endif