#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    char ch;
    printf("Input character:\n");
    read(stdin, ch, 1);
    ch=getchar();
    write(stdout, ch, 1);

    return EXIT_SUCCESS;
}