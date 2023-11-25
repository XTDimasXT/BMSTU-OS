#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/signal.h>

#define WAIT_READERS 0
#define ACTIVE_READERS 1
#define WAIT_WRITERS 2
#define ACTIVE_WRITER 3
#define BINARY_ACTIVE_WRITER 4

struct sembuf start_read[5] = {{WAIT_READERS, 1, 0}, {ACTIVE_WRITER, 0, 0}, 
{WAIT_WRITERS, 0, 0}, {WAIT_READERS, -1, 0}, {ACTIVE_READERS, 1, 0}};
struct sembuf stop_read[1] = {{ACTIVE_READERS, -1, 0}};

struct sembuf start_write[6] = {{WAIT_WRITERS, 1, 0}, {ACTIVE_READERS, 0, 0}, 
{ACTIVE_WRITER, 0, 0}, {BINARY_ACTIVE_WRITER, -1, 0}, {ACTIVE_WRITER, 1, 0}, {WAIT_WRITERS, -1, 0}};

struct sembuf stop_write[2] = {{ACTIVE_WRITER, -1, 0}, {BINARY_ACTIVE_WRITER, 1, 0}};

int flag = 1;

void sig_handler(int sig_num)
{
    flag = 0;
    printf("Signal %d catch by %d\n", sig_num, getpid());
}

void reader(int *shrm, int semid)
{
    srand(getpid());
    while (flag)
    {
        sleep(rand() % 2);

        if (semop(semid, start_read, 5) == -1)
        {
            perror("semop");
            exit(1);
        }

        int val = (int)(*shrm);
        printf("Reader %d, read value %d\n", getpid(), val);

        if (semop(semid, stop_read, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
    }

    exit(0);
}

void writer(int *shrm, int semid)
{
    srand(getpid());
    while (flag)
    {
        sleep(rand() % 3);

        if (semop(semid, start_write, 6) == -1)
        {
            perror("semop");
            exit(1);
        }

        (*shrm)++;

        printf("Writer %d, write value %d\n", getpid(), *shrm);

        if (semop(semid, stop_write, 2) == -1)
        {
            perror("semop");
            exit(1);
        }
    }

    exit(0);
}

int main()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("Can't signal\n");
        exit(1);
    }
    
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;

    int shm_fd = shmget(IPC_PRIVATE, 128, perms);
    if (shm_fd == -1)
    {
        perror("shmget");
        exit(1);
    }

    int *addr = (int *)shmat(shm_fd, 0, 0);
    *addr = 0;

    if (addr == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int sem_fd = semget(IPC_PRIVATE, 5, perms);
    if (sem_fd == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_fd, BINARY_ACTIVE_WRITER, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(sem_fd, ACTIVE_WRITER, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(sem_fd, WAIT_READERS, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(sem_fd, WAIT_WRITERS, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    if (semctl(sem_fd, ACTIVE_READERS, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(1);
    }

    pid_t child_pid;
    for (short i = 0; i < 5; ++i)
    {
        if ((child_pid = fork()) == -1)
        {
            perror("Can't fork.\n");
            exit(0);
        }
        else if (child_pid == 0)
        {
            reader(addr, sem_fd);
        }
    }

    for (short i = 0; i < 3; ++i)
    {
        if ((child_pid = fork()) == -1)
        {
            perror("Can't fork.\n");
            exit(0);
        }
        else if (child_pid == 0)
        {
            writer(addr, sem_fd);
        }
    }
    int wstatus;
    for (short i = 0; i < 8; ++i)
    {
        pid_t w = wait(&wstatus);
        if (w == -1)
        {
            perror("wait error");
            return 1;
        }

        if (WIFEXITED(wstatus))
        {
            printf("pid=%d, exited, status=%d\n", w, WEXITSTATUS(wstatus));
        }
        else if (WIFSIGNALED(wstatus))
        {
            printf("pid=%d, killed by signal %d\n", w, WTERMSIG(wstatus));
        }
        else if (WIFSTOPPED(wstatus))
        {
            printf("pid=%d, stopped by signal %d\n", w, WSTOPSIG(wstatus));
        }
    }

    if (shmctl(shm_fd, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        return 1;
    }

    if (shmdt((void *)addr) == -1)
    {
        perror("shmdt");
        return 1;
    }

    if (semctl(sem_fd, 0, IPC_RMID) == -1)
    {
        perror("semctl");
        return 1;
    }

    return 0;
}