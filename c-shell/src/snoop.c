#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "../include/token.h"

typedef struct {
    long number;
    int calls;
    double time;
    int first;
} Syscall;

Syscall stats[512];
int stat_count=0;
int first_order=0;
pid_t traced_pid=-1;

void handle_sigint(int)
{
    if(traced_pid>0)
        kill(traced_pid,SIGINT);
}

char* syscall_name(long n)
{
    switch(n) {
        case 0: return "read";
        case 1: return "write";
        case 2: return "open";
        case 3: return "close";
        case 4: return "stat";
        case 5: return "fstat";
        case 6: return "lstat";
        case 7: return "poll";
        case 8: return "lseek";
        case 9: return "mmap";
        case 10: return "mprotect";
        case 11: return "munmap";
        case 12: return "brk";
        case 13: return "rt_sigaction";
        case 14: return "rt_sigprocmask";
        case 21: return "access";
        case 35: return "nanosleep";
        case 39: return "getpid";
        case 41: return "socket";
        case 42: return "connect";
        case 44: return "sendto";
        case 45: return "recvfrom";
        case 56: return "clone";
        case 57: return "fork";
        case 59: return "execve";
        case 60: return "exit";
        case 61: return "wait4";
        case 62: return "kill";
        case 63: return "uname";
        case 89: return "readlink";
        case 158: return "arch_prctl";
        case 202: return "futex";
        case 218: return "set_tid_address";
        case 228: return "clock_gettime";
        case 231: return "exit_group";
        case 257: return "openat";
        case 262: return "newfstatat";
        case 273: return "set_robust_list";
        case 302: return "prlimit64";
        case 318: return "getrandom";
        default: {
            static char name[32];
            snprintf(name,sizeof(name),"syscall_%ld",n);
            return name;
        }
    }
}

Syscall* get_stat(long number)
{
    for(int i=0;i<stat_count;i++) {
        if(stats[i].number==number)
            return &stats[i];
    }

    stats[stat_count].number=number;
    stats[stat_count].calls=0;
    stats[stat_count].time=0;
    stats[stat_count].first=first_order++;

    return &stats[stat_count++];
}

double elapsed(struct timespec* start,struct timespec* end)
{
    return (end->tv_sec-start->tv_sec)
         + (end->tv_nsec-start->tv_nsec)/1000000000.0;
}

int compare(const void* a,const void* b)
{
    Syscall* x=(Syscall*)a;
    Syscall* y=(Syscall*)b;

    if(x->calls!=y->calls)
        return y->calls-x->calls;

    return x->first-y->first;
}

void print_summary()
{
    qsort(stats,stat_count,sizeof(Syscall),compare);

    printf("syscall\tcalls\ttime\n");

    for(int i=0;i<stat_count;i++) {
        printf("%s\t%d\t%.3fs\n",
               syscall_name(stats[i].number),
               stats[i].calls,
               stats[i].time);
    }
}

int trace(pid_t pid,int status)
{
    int entering=1;
    int signal_to_send=0;
    long number=0;
    struct timespec start,end;

    traced_pid=pid;

    if(ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD)<0)
        return 1;

    if(WIFSTOPPED(status))
        kill(pid,SIGCONT);

    while(1) {
        if(ptrace(PTRACE_SYSCALL,pid,0,signal_to_send)<0) {
            if(errno==ESRCH)
                break;
            return 1;
        }

        signal_to_send=0;

        if(waitpid(pid,&status,0)<0) {
            if(errno==EINTR)
                continue;
            return 1;
        }

        if(WIFEXITED(status) || WIFSIGNALED(status))
            break;

        if(!WIFSTOPPED(status))
            continue;

        if(WSTOPSIG(status)!=(SIGTRAP|0x80)) {
            int sig=WSTOPSIG(status);

            if(sig!=SIGTRAP && sig!=SIGSTOP)
                signal_to_send=sig;

            continue;
        }

        struct user_regs_struct regs;

        if(ptrace(PTRACE_GETREGS,pid,0,&regs)<0)
            return 1;

        if(entering) {
            number=regs.orig_rax;
            clock_gettime(CLOCK_MONOTONIC,&start);
            entering=0;
        } else {
            clock_gettime(CLOCK_MONOTONIC,&end);

            Syscall* s=get_stat(number);
            s->calls++;
            s->time+=elapsed(&start,&end);

            entering=1;
        }
    }

    traced_pid=-1;
    print_summary();

    return 0;
}

int snoop(Token* tokens,int size)
{
    if(size<2) {
        printf("snoop: invalid syntax\n");
        return 1;
    }

    sigset_t set,oldset;
    sigemptyset(&set);
    sigaddset(&set,SIGCHLD);

    sigprocmask(SIG_BLOCK,&set,&oldset);

    struct sigaction oldint,newint;
    memset(&newint,0,sizeof(newint));
    newint.sa_handler=handle_sigint;
    sigemptyset(&newint.sa_mask);

    sigaction(SIGINT,&newint,&oldint);

    stat_count=0;
    first_order=0;

    if(strcmp(tokens[1].text,"-p")==0) {
        if(size!=3) {
            printf("snoop: invalid syntax\n");
            sigaction(SIGINT,&oldint,NULL);
            sigprocmask(SIG_SETMASK,&oldset,NULL);
            return 1;
        }

        char* end;
        pid_t pid=(pid_t)strtol(tokens[2].text,&end,10);

        if(*end!='\0' || pid<=0 || kill(pid,0)<0) {
            printf("snoop: no such process\n");
            sigaction(SIGINT,&oldint,NULL);
            sigprocmask(SIG_SETMASK,&oldset,NULL);
            return 1;
        }

        if(ptrace(PTRACE_ATTACH,pid,0,0)<0) {
            if(errno==ESRCH)
                printf("snoop: no such process\n");
            else
                perror("ptrace");

            sigaction(SIGINT,&oldint,NULL);
            sigprocmask(SIG_SETMASK,&oldset,NULL);
            return 1;
        }

        int status;

        if(waitpid(pid,&status,0)<0) {
            sigaction(SIGINT,&oldint,NULL);
            sigprocmask(SIG_SETMASK,&oldset,NULL);
            return 1;
        }

        int ret=trace(pid,status);

        sigaction(SIGINT,&oldint,NULL);
        sigprocmask(SIG_SETMASK,&oldset,NULL);

        return ret;
    }

    char** argv=malloc(size*sizeof(char*));

    if(argv==NULL) {
        sigaction(SIGINT,&oldint,NULL);
        sigprocmask(SIG_SETMASK,&oldset,NULL);
        return 1;
    }

    for(int i=1;i<size;i++)
        argv[i-1]=tokens[i].text;

    argv[size-1]=NULL;

    pid_t pid=fork();

    if(pid<0) {
        free(argv);
        sigaction(SIGINT,&oldint,NULL);
        sigprocmask(SIG_SETMASK,&oldset,NULL);
        return 1;
    }

    if(pid==0) {
        sigprocmask(SIG_SETMASK,&oldset,NULL);

        signal(SIGINT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);

        if(ptrace(PTRACE_TRACEME,0,0,0)<0)
            _exit(1);

        raise(SIGSTOP);

        execvp(argv[0],argv);
        _exit(127);
    }

    int status;

    if(waitpid(pid,&status,0)<0) {
        free(argv);
        sigaction(SIGINT,&oldint,NULL);
        sigprocmask(SIG_SETMASK,&oldset,NULL);
        return 1;
    }

    if(WIFEXITED(status) && WEXITSTATUS(status)==127) {
        printf("snoop: command not found\n");
        free(argv);
        sigaction(SIGINT,&oldint,NULL);
        sigprocmask(SIG_SETMASK,&oldset,NULL);
        return 1;
    }

    int ret=trace(pid,status);

    free(argv);

    sigaction(SIGINT,&oldint,NULL);
    sigprocmask(SIG_SETMASK,&oldset,NULL);

    return ret;
}