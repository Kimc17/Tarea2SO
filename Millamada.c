#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

// Funciones del modo automático
void automatic_mode(char* name, char* param);
void search(char *call);
void show();

//Funciones del modo interactivo
int exec(int argc, char **argv);
int trace(pid_t child);
int parser(int syscall);
int find_syscall(pid_t child);

int main( int argc, char *argv[], char **envp )
{
    //Se obtienen el nombre del ejecutable, el modo y los parámetros (máximo 4)
    char* name = argv[1];
    char* mode = argv[2];
    char param[50] = " ";
    if (argc > 2){
        for( int count = 3; count < argc; count++ ){
        sprintf(param, "%s %s",param, argv[count]);
        }  
    }  
    //Se verifica el módulo de ejecución
    if (!strcmp(mode, "automatic")){
        automatic_mode(name, param);       
    }
    else if (!strcmp(mode, "interactive")){
        //Se crean dos procesos
        pid_t child = fork();
        if (child == 0) {
            //Ejecuta el programa
            return exec(argc-1, &name);
        } else {
            //Rastrea el programa
            return trace(child);
        }
    }
    else {
        printf("Please enter a valid mode!\n");
        exit(-1);
    }
        
    return 0;
}

void automatic_mode(char* name, char* param)
{
    char program[50];
    int total=0;
    //Ejecución de la función strace
        if (param == " "){
            sprintf(program,"strace -o info.txt %s\n", name);
        }else{
            sprintf(program,"strace -o info.txt %s %s\n", name, param);
        }
        
        char strace = system(program); 
        if (strace==1){
            printf("An error has occurred");
            exit(-1);
        }
        //Se abre el archivo con la información del strace
        FILE *file, *file2; 
        char content[100];
        file = fopen("info.txt","r");  
        if (file == NULL){
            printf("An error has occurred");
            exit(-1);
        }
        else {
            //Se cuentan la cantidad de líneas
            int lines = 0;
            while (feof(file) == 0) {
                fgets(content,100,file);
                lines ++;
            }
            fclose(file);
            //Se buscan las palabras claves por llamada al sistema
            file2 = fopen("info.txt","r");
            char strs[lines][10]; 
            int i = 0;
            
            
            while (feof(file2) == 0) {
                fgets(content,100,file);
                if(strstr(content, "(") != NULL){
                    char *call = strtok(content, "(");
                    strcpy(strs[i], call);    
                    i++;      
                }
            }
            int count = 0;
            FILE *fp;
            char cadena[] = "Syscall | Amount   \n";
            fp = fopen ("out.txt", "w");    
            fputs(cadena, fp);                  
            while (count < i) {
                int j = 0;
                int cont = 0;
                if (strcmp(strs[count], " ") != 0){
                     
                    while (j < i) {       
                        if(strcmp(strs[count], strs[j]) == 0){
                            cont ++;        
                            if(count != j){
                                strcpy(strs[j], " "); 
                            }     
                        }
                        j++;
                    }
                    char num2[30] = "";
                    sprintf(num2,"%s|%d\n", strs[count], cont);
                    fputs(num2, fp);
                    total += cont;
                    
                }
                count ++;
            }
            char last [20] = "";
            sprintf(last,"Total|%d\n", total);
            fputs(last, fp);
            fclose(file2);
            fclose (fp);   
            show();        
}
}
void show(){
    FILE *archivo;
 	char caracteres[100];
 	archivo = fopen("out.txt","r");
 	if (archivo == NULL)
 		exit(1);
 	else{
 	    printf("\n\n");
 	    while (feof(archivo) == 0){
 		fgets(caracteres,100,archivo);
 		printf("%s",caracteres);
 	    }
    }
    fclose(archivo);
}

// MODO INTERACTIVO 
int exec(int argc, char **argv) {
    char *args [argc+1];
    memcpy(args, argv, argc * sizeof(char*));
    args[argc] = NULL;
    //Se inicia el procesos de rastreo (PTRACE_TRACEME: el proceso sabe que quiere ser rastreado)
    ptrace(PTRACE_TRACEME);
    //Detiene el proceso para que el padre comience a rastrear y continuar la ejecución
    kill(getpid(), SIGSTOP);
    // COn execvp se inicia el nuevo programa 
    return execvp(args[0], args);
}
int trace(pid_t child) {
    int status, syscall, retval;
    waitpid(child, &status, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
    while(1) {
        if (find_syscall(child) != 0) break;
        syscall = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*ORIG_RAX);
        parser(syscall);
    }
    return 0;
}
int find_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}
int parser(int syscall) {
    char sys[20] = ""; 
    sprintf(sys, "%d", syscall);
    FILE *file; 
    char *num = "";
    char *name = "";
    char content[100];
    file = fopen("syscall_64.tbl","r");
    while (feof(file) == 0) {
        fgets(content,100,file);
        num = strtok(content, " ");
        if(num != NULL){
            name = strtok(NULL, " ");
        }
        if (strcmp(sys, num) == 0){
            printf("Ejecutando syscall %s\n", name);
        }
    }
}