#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define INCORRECT_LEN 1

int main(void)
{
    int len;
    printf("Введите длину вектора:\n");
    if (scanf("%d", &len) != 1 || len <= 0)
    {
        printf("Некорректный ввод длины (Длина должна быть > 0)\n");
        return INCORRECT_LEN;
    }

    return EXIT_SUCCESS;
}