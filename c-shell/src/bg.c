#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "../include/bg.h"

Job jobs[100];
int job_count = 0;
int next_job_number = 1;

void sighup()
{
    for(int i = 0; i < job_count; i++)
    {
        Job *job = &jobs[i];
        if(job->state == -1)
        {
            kill(-job->pgid, SIGHUP);
        }
    }
    for(int x=0; x<job_count; x++)
    {
        for(int y=0; y<jobs[x].process_count; y++)
        {
            Process *process = &jobs[x].processes[y];
            if(!process->done)
            {
                kill(process->pid, SIGHUP);
            }
        }
    }
}

int spdJobs()
{
    for(int x=0; x<job_count; x++)
    {
        Job* j = &jobs[x];
        if(j-> state == -1)
        {
            return 1;
        }
    }
    return 0;
}

void stop(int job_number)
{
    jobs[job_number-1].state = -1;
}

int add(pid_t pgid, pid_t reported_pid, Process *processes, int count)
{
    if(job_count >= 100)
    {
        printf("cshell: Maximum number of jobs reached\n");
        return -1;
    }

    Job *job = &jobs[job_count];

    job->job_n = next_job_number++;
    job->pgid = pgid;
    job->reported_pid = reported_pid;
    job->process_count = count;
    job->state = 0; 

    for(int x = 0; x < count; x++)
    {
        job->processes[x] = processes[x];
    }

    job_count++;

    return job->job_n;
}

void sig_handler(int sig)
{
    (void)sig;
    for(int i = 0; i < job_count; i++)
    {
        for(int j = 0; j < jobs[i].process_count; j++)
        {
            Process *process = &jobs[i].processes[j];
            int status;
            pid_t pid;

            if(process->done)
            {
                continue;
            }

            pid = waitpid(process->pid, &status, WNOHANG);
            if(pid == process->pid)
            {
                process->done = 1;
                process->status = status;
                if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    printf("%s with pid %d exited normally\n", process->command, pid);
                }
                else
                {
                    printf("%s with pid %d exited abnormally\n", process->command, pid);
                }
            }
        }
    }
}

int activities(void)
{
    for(int i = 0; i < job_count; i++)
    {
        Job *job = &jobs[i];
        int active = 0;

        for(int j = 0; j < job->process_count; j++)
        {
            if(!job->processes[j].done)
            {
                active = 1;
                break;
            }
        }

        if(!active)
        {
            continue;
        }

        printf("[%d] pgid %d\n", job->job_n, job->pgid);

        for(int j = 0; j < job->process_count; j++)
        {
            Process *process = &job->processes[j];
            if(job -> state == -1)
            {
                printf("  %d %s Stopped\n", process->pid, process->command);
            }
            else if(!process->done)
            {
                printf("  %d %s Running\n",
                       process->pid,
                       process->command);
            }
        }
    }

    return 0;
}