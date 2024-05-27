#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>


void handle_Control_C(int signal) {
    printf("\nYou typed Control-C!\n");
}

int main() {
signal(SIGINT, handle_Control_C);

int numCommand = 0;
char command[1024];
char lastCommands[21][1024];

char var[1024];
int count_var = 0;
char variables[20][1024];
char variables_val[20][1024];

char *token;
int i;
char *outfile;
int fd, amper, redirect, piping, retid, status, argc1, redirectError, redirectAppend;
int fildes[2];
char *argv1[10], *argv2[10];

char prompt[1024];
strcpy(prompt, "hello:");

while (1)
{
    printf("%s ", prompt);
    fgets(command, 1024, stdin);
    command[strlen(command) - 1] = '\0';

    /* navigate between the commands: arrow up or down */
    if (command[0] == '\033') {
        int point_command = numCommand+1; 
        for(int i = 0; command[i] != '\0'; i++) { 
            if (command[i] == '\033' && command[i+1] == '[') {
                switch(command[i+2]) { 
                    case 'A': // code for arrow up
                        point_command--;
                        break;;
                    case 'B': // code for arrow down
                        if(point_command != numCommand+1) {
                            point_command++;
                        }
                        break;;
                }
            } else {
                continue;
            }
        }
        if(point_command <= numCommand && point_command > numCommand - 20 && point_command > 0) { 
            printf("%s\n", lastCommands[point_command%20]);
            strcpy(command,lastCommands[point_command%20]);
        } else {
            printf("you don't have access to this command!\n");
            continue;
        }
    }

    /* !! command: do the last command */
    if(!strcmp(command, "!!")) {
        if(numCommand > 0) {
            strcpy(command, lastCommands[numCommand%20]);
            printf("%s\n", command);
        } else {
            continue;
        }
    } 
    
    /* for commands history */
    numCommand = numCommand +1;
    strcpy(lastCommands[numCommand%20] , command);
    
    /* if command */
    if (command[0] == 'i' && command[1] == 'f') {
        int see_fi = 1;
        command[strlen(command)-1] = '\n';
        char next_if[1024];
        while (see_fi) {
            memset(next_if, 0, sizeof(next_if)); 
            fgets(next_if, 1024, stdin);
            strcat(command, next_if);
            command[strlen(command) - 1] = '\n';
            if (!strcmp(next_if, "fi\n")) {
                see_fi = 0;
            }
        }
        if (fork()==0) {
            char* argss[] = {"bash", "-c", command, NULL};
            execvp(argss[0], argss);
        }
        wait(&status);
        continue;
    }
    
    /* pipe command */
    else if ((strstr(command, " | ") != NULL)) {
        int pipe_count = 0;
        char **pipe_command = (char**) malloc(sizeof(char*) * 10);
        int size = 10;
        token = strtok(command, "|");
        while (token != NULL) {
            pipe_command[pipe_count] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(pipe_command[pipe_count], token);
            pipe_count++;
            if (pipe_count >= size-1) { // resize array if necessary
                pipe_command = (char**) realloc(pipe_command, sizeof(char*) * (size + 10));
                size+=10;
            }
            token = strtok(NULL, "|");
        }
        pipe_command[pipe_count] = NULL;
        
        int pipes[2];
        int input = STDIN_FILENO; 
        
        for (int j = 0; j < pipe_count; j++) {
            int argv_count = 0;
            char **argv = (char**) malloc(sizeof(char*) * 10);
            size = 10;
            token = strtok(pipe_command[j], " ");
            while (token != NULL) {
                argv[argv_count] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(argv[argv_count], token);
                argv_count++;
                if (argv_count >= size-1) { 
                    argv = (char**) realloc(argv, sizeof(char*) * (size + 10));
                    size+=10;
                }
                token = strtok(NULL, " ");
            }
            argv[argv_count] = NULL;

            pipe(pipes); 

            if (fork() == 0) { 
                close(pipes[0]); 
                dup2(input, STDIN_FILENO); 
                if (j < pipe_count - 1) {
                    dup2(pipes[1], STDOUT_FILENO);
                }
                close(pipes[1]); 

                if (execvp(argv[0], argv) == -1) {
                    printf("Error executing command %s\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
            } else { 
                close(pipes[1]); 
                if (input != STDIN_FILENO) {
                    close(input); 
                }
                input = pipes[0]; 
                wait(NULL);
                free(argv);
            }
        }
        for (int j = 0; j < pipe_count; j++) {
            free(pipe_command[j]);
        }
        free(pipe_command);
        continue;
    }    

    piping = 0;

    /* parse command line */
    i = 0;
    token = strtok (command," ");
    while (token != NULL)
    {
        argv1[i] = token;
        token = strtok (NULL, " ");
        i++;
        if (token && ! strcmp(token, "|")) {
            piping = 1;
            break;
        }
    }
    argv1[i] = NULL;
    argc1 = i;

    /* Is command empty */
    if(argv1[0] == NULL)
        continue;

    /* quit command */
    if(! strcmp(argv1[0], "quit")) {
        return 0;
    }

    /* Does command contain pipe */
    if (piping) {
        i = 0;
        while (token!= NULL)
        {
            token = strtok (NULL, " ");
            argv2[i] = token;
            i++;
        }
        argv2[i] = NULL;
    }

    /* Does command line end with & */ 
    if (! strcmp(argv1[argc1 - 1], "&")) {
        amper = 1;
        argv1[argc1 - 1] = NULL;
        }
    else 
        amper = 0;     

    /* chanching the prompt */ 
    if(argc1 > 2 && ! strcmp(argv1[argc1 - 2], "=") && ! strcmp(argv1[argc1 - 3], "prompt")) {
        strcpy(prompt, argv1[argc1 - 1]);
        continue;
    }

    /* read command */ 
    if(! strcmp(argv1[0], "read")) {
        strcpy(variables[count_var%20], "$");
        strcat(variables[count_var%20],argv1[1]);
        fgets(var, 1024, stdin); 
        var[strlen(var) - 1] = '\0';
        strcpy(variables_val[count_var%20], var);
        count_var++;
        continue;
    }

    /* add variables */ 
    if(argc1 > 1 && argv1[0][0] == '$' && ! strcmp(argv1[1], "=")) {
        strcpy(variables[count_var%20], argv1[0]);
        strcpy(variables_val[count_var%20], argv1[2]);
        count_var++;
        continue;
    }

    /* echo command */     
    if(! strcmp(argv1[0], "echo")) {
        //status of last command
        if(! strcmp(argv1[1], "$?")) {
            printf("%d\n", status);
        //print value of variable
        } else if (argv1[1][0] == '$' && strcmp(argv1[1], "$")) {
            for(int i = 0; i < count_var && i < 20; i++) {
                if(!strcmp(variables[i], argv1[1])) {
                    printf("%s\n", variables_val[i]);
                }
            }
        } else {
            //echo 
            for (int j = 1; j < argc1; j++) {
                printf("%s ", argv1[j]);
            }
            printf("\n");
        }
        continue;
    }

    /* cd command */     
    if(! strcmp(argv1[0], "cd")) {
        chdir(argv1[1]);
        continue;
    }
    
    /* chanching the stdout */ 
    if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">")) {
        redirect = 1;
        redirectAppend = 0;
        redirectError = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    /* chanching the stderror */ 
    } else if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], "2>")) {
        redirectError = 1;
        redirect = 0;
        redirectAppend = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    /* do append for a file */ 
    } else if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">>")) {
        redirectAppend = 1;
        redirect = 0;
        redirectError = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    } else {
        redirectAppend = 0;
        redirectError = 0;
        redirect = 0;  
    }
    
    /* for commands not part of the shell command language */ 
    if (fork() == 0) { 
        /* redirection of IO ? */
        if (redirect || redirectAppend) {
            if(redirect) {
                fd = creat(outfile, 0660); 
            } else {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
            }
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stdout is now redirected */
        }
        if (redirectError) {
            fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
            close (STDERR_FILENO) ; 
            dup(fd); 
            close(fd);  
            /* std error is now redirected */
        } 
        if (piping) {
            pipe (fildes);
            if (fork() == 0) { 
                /* first component of command line */ 
                close(STDOUT_FILENO); 
                dup(fildes[1]); 
                close(fildes[1]); 
                close(fildes[0]); 
                /* stdout now goes to pipe */ 
                /* child process does command */ 
                execvp(argv1[0], argv1);
            } 
            /* 2nd command component of command line */ 
            close(STDIN_FILENO);
            dup(fildes[0]);
            close(fildes[0]); 
            close(fildes[1]); 
            /* standard input now comes from pipe */ 
            execvp(argv2[0], argv2);
        } 
        else {
            execvp(argv1[0], argv1);
        }
    }
    /* parent continues over here... */
    /* waits for child to exit if required */
    if (amper == 0)
        retid = wait(&status);
}
}
