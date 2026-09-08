#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "../include/bg.h"
#include "../include/token.h"

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

int add(
    pid_t pgid,
    pid_t reported_pid,
    Process *processes,
    int count,
    const char *cmd
)
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
    strncpy(job->cmd, cmd, sizeof(job->cmd) - 1);
    job->cmd[sizeof(job->cmd) - 1] = '\0';

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
volatile sig_atomic_t timed_out = 0;

void alarm_handler(int sig)
{
    (void)sig;
    timed_out =1;
}

int resume_job(int jn, int bg, int timeout)
{
    if(jn > job_count || jobs[jn-1].state != -1)
    {
        printf("resume: Stopped job not found\n");
        return 1;
    }
    Job* job = &jobs[jn-1];
    if(bg)
    {
        if(kill(-job->pgid, SIGCONT) < 0)
        {
            printf("resume: No such job\n");
            return 1;
        }
        job->state = 0;
        printf("[%d] Running %s\n", job->job_n, job->cmd);
        return 0;
    }
    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &oldmask);
    printf("%s\n", job->cmd);
    tcsetpgrp(STDIN_FILENO, job->pgid);
    if(kill(-job->pgid, SIGCONT) < 0)
    {
        printf("resume: No such job\n");
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        return 1;
    }
    job->state = 0;
    timed_out = 0 ;
    if(timeout > 0)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = alarm_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);
        alarm(timeout);
    }
    int stopped = 0;

    for(int x = 0; x < job->process_count; x++)
    {
        if(job->processes[x].done)
            continue;

        int status;

        pid_t result = waitpid(
            job->processes[x].pid,
            &status,
            WUNTRACED
        );

        if(result < 0)
        {
            if(errno == EINTR && timed_out)
            {
                kill(-job->pgid, SIGTERM);
                printf("resume: job timed out\n");

                job->state = 1;

                alarm(0);
                tcsetpgrp(STDIN_FILENO, shell_pgid);
                sigprocmask(SIG_SETMASK, &oldmask, NULL);

                return 0;
            }

            if(errno == ECHILD)
            {
                job->processes[x].done = 1;
                continue;
            }

            continue;
        }

        if(WIFSTOPPED(status))
        {
            job->state = -1;
            stopped = 1;
            break;
        }

        if(WIFEXITED(status) || WIFSIGNALED(status))
        {
            job->processes[x].done = 1;
            job->processes[x].status = status;
        }
    }
    if(!stopped)
    {
        int all_done = 1;

        for(int x = 0; x < job->process_count; x++)
        {
            if(!job->processes[x].done)
            {
                all_done = 0;
                break;
            }
        }

        if(all_done)
            job->state = 1;
    }
    alarm(0);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    return 0;
}