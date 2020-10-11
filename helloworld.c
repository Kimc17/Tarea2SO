#include <stdio.h>
#include <stdlib.h>
int main( int argc, char *argv[], char **envp )
{
    if (argc<2){
        printf("Ingrese un nombre!");
        exit(1);
    }
    printf("Hola %s\n", argv[1]);
}