#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
 
#define SIZE_BUF 128
#define PRODUCERS_AMOUNT 3
#define CONSUMERS_AMOUNT 3
#define PERMS S_IRWXU | S_IRWXG | S_IRWXO
#define SEQ_LEN 30
 
char *seq_start;
 
char *addr;
 
char **ptr_prod;
char **ptr_cons;
char *cur_alpha;
 
#define SB 0
#define SE 1
#define SF 2
 
#define P -1
#define V 1
 
struct sembuf start_produce[2] = {{SE, P, 0}, {SB, P, 0}};
struct sembuf stop_produce[2] = {{SB, V, 0}, {SF, V, 0}};
struct sembuf start_consume[2] = {{SF, P, 0}, {SB, P, 0}};
struct sembuf stop_consume[2] = {{SB, V, 0}, {SE, V, 0}};
 
int flag_active = 1;
 
void sig_handler(int sig_num)
{
    flag_active = 0;
    printf("sig_catch: %d,pid: %d\n", sig_num, getpid());
}
 
void produce(int sem_id)
{
 
    while (flag_active)
    {
        usleep(1.0 * rand() / RAND_MAX * 1000000);
        int sem_op_p = semop(sem_id, start_produce, 2);
        if (sem_op_p == -1)
        {
            perror("semop error\n");
            exit(1);
        }
 
        **ptr_prod = *cur_alpha;
 
        if (*cur_alpha > 'z')
        {
 
            printf("Producer %d - work finished\n", getpid());
            int sem_op_v = semop(sem_id, stop_produce, 2);
            return;
        }
 
        printf("Producer %d - created %c on %p\n", getpid(), **ptr_prod, *ptr_prod);
 
        (*cur_alpha)++;
        (*ptr_prod)++;
 
        int sem_op_v = semop(sem_id, stop_produce, 2);
 
        if (sem_op_v == -1)
        {
            perror("semop error\n");
            exit(1);
        }
    }
}
 
void consume(int sem_id)
{
    while (flag_active)
    {
        usleep(1.0 * rand() / RAND_MAX * 1000000);
        int sem_op_p = semop(sem_id, start_consume, 2);
        if (sem_op_p == -1)
        {
            perror("semop error\n");
            exit(1);
        }
        if (**ptr_cons > 'z')
        {
            printf("Consumer %d - work finished\n", getpid());
            int sem_op_v = semop(sem_id, stop_consume, 2);
            return;
        }
 
        printf("Consumer %d - read %c on %p\n", getpid(), **ptr_cons, *ptr_cons);
 
        (*ptr_cons)++;
 
        int sem_op_v = semop(sem_id, stop_consume, 2);
 
        if (sem_op_v == -1)
        {
            perror("semop error\n");
            exit(1);
        }
    }
}
 
int main()
{
    int shmid, semid;
    key_t sem_key = 100;
    key_t shm_key = 200;
    shmid = shmget(shm_key, SIZE_BUF * sizeof(char), IPC_CREAT | PERMS);
    if (shmid == -1)
    {
        perror("shmget error");
        exit(1);
    }
 
    addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1)
    {
        perror("Cannot attach\n");
        exit(1);
    }
 
    ptr_prod = (char **)addr;
    ptr_cons = ptr_prod + 1;
    cur_alpha = ptr_cons + 1;
 
    seq_start = cur_alpha + 1;
 
    *ptr_prod = seq_start;
    *ptr_cons = seq_start;
 
    *cur_alpha = 'a';
 
    if ((semid = semget(sem_key, 3, IPC_CREAT | PERMS)) == -1)
    {
        perror("semget error\n");
        exit(1);
    }
    int c_sb = semctl(semid, SB, SETVAL, 1);
    int c_se = semctl(semid, SE, SETVAL, SEQ_LEN);
    int c_sf = semctl(semid, SF, SETVAL, 0);
 
    if (c_se == -1 || c_sf == -1 || c_sb == -1)
    {
        perror("Cannot semctl");
        exit(1);
    }
 
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("Cannot change signal.\n");
        exit(EXIT_FAILURE);
    }
 
    int pid = -1;
 
    for (int i = 0; i < CONSUMERS_AMOUNT; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("can't fork\n");
            exit(1);
        }
 
        if (pid == 0)
        {
            consume(semid);
            exit(0);
        }
    }
 
    for (int i = 0; i < PRODUCERS_AMOUNT; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("can't fork\n");
            exit(1);
        }
 
        if (pid == 0)
        {
            produce(semid);
            exit(0);
        }
    }
 
    for (int i = 0; i < (CONSUMERS_AMOUNT + PRODUCERS_AMOUNT); i++)
    {
        int wstatus;
        pid_t wpid = wait(&wstatus);
        if (WIFEXITED(wstatus))
        {
            printf("Child PID is : %d exited, status=%d\n", wpid, WEXITSTATUS(wstatus));
        }
        else if (WIFSIGNALED(wstatus))
        {
            printf("Child PID is : %d killed by signal %d\n",wpid, WTERMSIG(wstatus));
        }
        else if (WIFSTOPPED(wstatus))
        {
            printf("Child PID is : %d stopped by signal %d\n",wpid, WSTOPSIG(wstatus));
        }
        else if (WIFCONTINUED(wstatus))
        {
            printf("Child PID is : %d continued\n",wpid);
        }
    }
 
    if (shmdt(addr) == -1)
    {
        perror("Cannot detach\n");
        exit(1);
    }
 
    if (shmctl(shmid, IPC_RMID, (void *)addr) < 0)
    {
        perror("Cannot clear shared memory\n");
        exit(1);
    }
 
    if (semctl(semid, 0, IPC_RMID) < 0)
    {
        perror("Cannot clear semaphore memory\n");
        exit(1);
    }
}