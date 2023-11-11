#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>

static int flag = 0;


void sig_handler(int sig_numb)
{
    flag = 1;
    printf("\nSignal %d was received.\n", sig_numb);
}

int main() {
    pid_t childpid[2];
    char messages[2][100] = {"aaaaa\n", "bbbbbbbbbbbbbbbbb\n"};
    int fd[2];
    if (pipe(fd) == -1) 
    {
        perror("Can`t pipe\n");
        exit(1);
    }
    char ch;

    signal(SIGINT, sig_handler);
    sleep(2);

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
            if (flag) 
            {
                close(fd[0]);
                write(fd[1], messages[i], sizeof(messages[i]));
                printf("%d sent message: %s", getpid(), messages[i]);
            }
            exit(0);
        }
    }

    pid_t waited_child;
    for (int i = 0; i < 2; i++) {
        waited_child = waitpid(childpid[i], &status, WCONTINUED);
        if (waited_child == -1) {
            printf("Can't wait\n");
            return 1;
        }
        printf("childpid = %d, parentpid = %d\n", waited_child, getpid());
        if (WIFEXITED(status)) {
            printf("exited, status=%d\n\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d\n\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d\n\n", WSTOPSIG(status));
        }
        close(fd[1]);
        printf("read message: ");
        while(read(fd[0], &ch, 1) > 0 && ch != '\n') 
            printf("%c", ch);
        printf("\n");
    }
}