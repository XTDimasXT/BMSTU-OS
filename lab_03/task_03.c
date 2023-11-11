#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>


int main(void) 
{
    pid_t childPids[3];

    char *path1 = "./task_03_1.o";
    char *path2 = "./task_03_2.o";
    char *path3 = "./task_03_3.o";

    char *paths[3] = {path1, path2, path3};

    for (int i = 0; i < 3; i++)  
    {
        if ((childPids[i] = fork()) == -1) 
        {
            printf("Cant't fork\n");
            exit(1);
        } 
        else if (childPids[i] == 0) 
        {
            if (execv(paths[i], NULL)==-1)
            {
                perror("exec");
                exit(1);
            }
        } 

        else 
        {
            printf("Parent: child=%d, pid = %d, grid = %d\n",childPids[i], getpid(), getpgrp());
        }
    }
    
    int status;
    
    for (int i = 0; i < 3; i++)
    {
        pid_t wPid = waitpid(childPids[i], &status, WUNTRACED);
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
}
