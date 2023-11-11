#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#define N 2

int main(void)
{
    pid_t childpid[N];
    
    for (size_t i = 0; i < N; i++)
    {
        childpid[i] = fork();
	    if (childpid[i] == -1)
	    {
	        perror("Can't fork\n");
	        exit(1);
	    }
	    else if (childpid[i] == 0)
	    {
            printf("Child process before sleep: pid = %d, ppid = %d, grid = %d\n",
	                getpid(), getppid(), getpgrp());
	        sleep(2);
	        printf("\nChild process after sleep: pid = %d, ppid = %d, grid = %d\n",
	                getpid(), getppid(), getpgrp());
	        return 0;
	    }
	    else 
	    {
	        printf("Parent process: pid = %d, grid = %d\n", 
        	        getpid(),getpgrp());
	    }
    }
    
    return 0;
}