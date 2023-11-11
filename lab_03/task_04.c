#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void clear_buf(char *buf) 
{
    for (size_t i = 0; i < sizeof(buf); i++)
        buf[i] = 0;
}


int main()
{
    pid_t childpid[2];
    char messages[2][100] = {"aaaaa\n", "bbbbbbbbbb\n"};
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("Can`t pipe\n");
        exit(1);
    }
    char buf[100];

    int status;
    for (int i = 0; i < 2; i++) 
    {
        if ((childpid[i]=fork()) == -1) 
        {
            perror("Can`t fork\n");
            exit(1);
        } 
        else if (childpid[i] == 0) 
        {
            printf("Child: pid = %d, ppid = %d, grid = %d\n", getpid(), getppid(), getpgrp());
            close(fd[0]);
            write(fd[1], messages[i], sizeof(buf));
            printf("%d sent message: %s", getpid(), messages[i]);
            exit(0);
        } 
        else
        {
            clear_buf(buf);
            read(fd[0], buf, sizeof(buf));
            printf("%d read message: %s\n", getpid(), buf);
        }
    }

    for (int i = 0; i < 2; i++) 
    {
        pid_t wPid = waitpid(childpid[i], &status, WUNTRACED);
        if (wPid == -1) 
        {
            printf("Can't wait\n");
            exit(1);
        }
        if (WIFEXITED(status))
            printf("%d exited, status=%d\n", wPid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("%d killed by signal %d\n", wPid, WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("%d stopped by signal %d\n", wPid, WSTOPSIG(status));
    }

    clear_buf(buf);
    close(fd[1]);
    read(fd[0], buf, sizeof(buf));
    printf("%d read message: %s\n", getpid(), buf);
}