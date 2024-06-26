#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

// Prompt initialization
char prompt[1024] = "hello:";

// This function handles the SIGINT signal, which is generated when the user
// presses Ctrl+C in the terminal.
void handle_Control_C(int signal) {
    // Print a message indicating that the user typed Control-C.
    printf("\nYou typed Control-C!\n");

    printf("%s ", prompt);
    // Flush the output buffer to ensure the prompt is printed immediately
    fflush(stdout);
}

int main() {
// Registering handle_Control_C function to handle SIGINT signal (Ctrl+C).
signal(SIGINT, handle_Control_C);

int numCommand = 0; // Counter for the number of commands entered
char command[1024]; // Array to store the entered command
char lastCommands[21][1024]; // Array to store the last 20 commands entered

char var[1024]; // Buffer for reading user input
int count_var = 0; // Counter for the number of user-defined variables
char variables[20][1024]; // Array to store variable names
char variables_val[20][1024]; // Array to store variable values

// Token and index variables for command parsing
char *token;
int i;

// Variables for redirection, piping, and background execution
char *outfile; // File for output redirection
int fd, amper, redirect, piping, retid, status, argc1, redirectError, redirectAppend;
int fildes[2]; // File descriptors for piping
char *argv1[10], *argv2[10]; // Arrays to store command arguments

while (1)
{
    // Print the prompt
    printf("%s ", prompt);
    // Read the command entered by the user
    fgets(command, 1024, stdin);
    command[strlen(command) - 1] = '\0';

    /* navigate between the commands: arrow up or down */
    if (command[0] == '\033') { //indicates that the user pressed an arrow key
        int point_command = numCommand+1; //initializes to the index of the next command in the history
        for(int i = 0; command[i] != '\0'; i++) { 
            if (command[i] == '\033' && command[i+1] == '[') {
                switch(command[i+2]) { 
                    case 'A': // code for arrow up
                        point_command--; // navigate to the previous command in the history
                        break;;
                    case 'B': // code for arrow down
                        if(point_command != numCommand+1) {
                            point_command++; // navigate to the next command in the history
                        }
                        break;;
                }
            } else {
                // skip the rest of the loop iteration if the current character does 
                // not match the conditions for an arrow key escape sequence
                continue;
            }
        }
        // Check if the command index is within the valid range of the command history array.
        if(point_command <= numCommand && point_command > numCommand - 20 && point_command > 0) { 
            // Print the command from the history corresponding to the adjusted index.
            printf("%s\n", lastCommands[point_command%20]);
            // Replace the current command with the selected command from the history
            strcpy(command,lastCommands[point_command%20]);
        } else {
            // Inform the user if the requested command is out of range or invalid.
            printf("\033[0;31mthe requested command is out of range!\033[0m\n");
            // printf("you don't have access to this command!\n");
            continue;
        }
    }

    /* !! command: do the last command */
    if(!strcmp(command, "!!")) {
        // If there are previous commands in history, execute the last one
        if(numCommand > 0) {
            // Copy the last command from history to the current command buffer
            strcpy(command, lastCommands[numCommand%20]);
            printf("%s\n", command);
        } else {
            // If there are no previous commands, skip execution and prompt for a new command.
            continue;
        }
    } 
    
    /* for commands history */
    numCommand = numCommand +1;
    strcpy(lastCommands[numCommand%20] , command);
    
    /* if command */
    if (command[0] == 'i' && command[1] == 'f') {
        int see_fi = 1; // set the flag
        command[strlen(command)-1] = '\n';
        char next_if[1024]; // Initialize a buffer to store the next line of the command

        // Continue reading subsequent lines until 'fi' is encountered
        while (see_fi) {
            memset(next_if, 0, sizeof(next_if)); // Clear the buffer
            fgets(next_if, 1024, stdin); // Read the next line of the command from stdin
            strcat(command, next_if); // Concatenate the next line to the current command
            command[strlen(command) - 1] = '\n';

            // Check if the current line is 'fi' indicating the end of the if block
            if (!strcmp(next_if, "fi\n")) {
                see_fi = 0; // change the flag
            }
        }
        if (fork()==0) {
            char* argss[] = {"bash", "-c", command, NULL};
            execvp(argss[0], argss);
        }
        wait(&status); // Wait for the child process to finish
        continue;
    }
    
    /* pipe command */
    else if ((strstr(command, " | ") != NULL)) {
        int pipe_count = 0;
        char **pipe_command = (char**) malloc(sizeof(char*) * 10);
        int size = 10;
        token = strtok(command, "|");

        // Tokenize the command into pipe commands
        while (token != NULL) {
            pipe_command[pipe_count] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(pipe_command[pipe_count], token); // Copy the tokenized pipe command
            pipe_count++;

            if (pipe_count >= size-1) { // resize array if necessary
                pipe_command = (char**) realloc(pipe_command, sizeof(char*) * (size + 10));
                size+=10;
            }
            token = strtok(NULL, "|");
        }
        pipe_command[pipe_count] = NULL;
        
        // Initialize pipes
        int pipes[2];
        int input = STDIN_FILENO; 
        
        // Iterate through each pipe command
        for (int j = 0; j < pipe_count; j++) {
            int argv_count = 0;
            char **argv = (char**) malloc(sizeof(char*) * 10);
            size = 10;
            token = strtok(pipe_command[j], " ");

            // Tokenize the pipe command into arguments
            while (token != NULL) {
                argv[argv_count] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(argv[argv_count], token);
                argv_count++;

                if (argv_count >= size-1) {  // Resize the argv array if necessary
                    argv = (char**) realloc(argv, sizeof(char*) * (size + 10));
                    size+=10;
                }
                token = strtok(NULL, " ");
            }
            argv[argv_count] = NULL;

            pipe(pipes); 

            // Fork a child process for executing the pipe command
            if (fork() == 0) { 
                close(pipes[0]); // Close the read end of the pipe
                dup2(input, STDIN_FILENO); // Redirect input to the pipe
                // If not the last pipe command, redirect output to the pipe
                if (j < pipe_count - 1) {
                    dup2(pipes[1], STDOUT_FILENO);
                }
                close(pipes[1]); 

                // Execute the command
                if (execvp(argv[0], argv) == -1) {
                    printf("Error executing command %s\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
            } else { 
                close(pipes[1]); // Close the write end of the pipe
                // Close input if not stdin
                if (input != STDIN_FILENO) {
                    close(input); 
                }
                input = pipes[0]; // Update input for the next pipe command
                wait(NULL); // Wait for child process to finish
                free(argv);
            }
        }
        for (int j = 0; j < pipe_count; j++) {
             // Free memory allocated for pipe commands
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
            piping = 1; // Set the piping flag to indicate that the command contains a pipe
            break;
        }
    }
    argv1[i] = NULL;
    argc1 = i; // Store the number of arguments

    /* Is command empty */
    if(argv1[0] == NULL)
        continue;

    /* quit command */
    if(! strcmp(argv1[0], "quit")) {
        return 0;
    }

    /* Does command contain pipe */
    if (piping) { // piping flag is set
        i = 0;
        while (token!= NULL)
        {
            token = strtok (NULL, " ");
            argv2[i] = token; // Store the token
            i++; // Move to the next index in the argv2 array
        }
        argv2[i] = NULL;
    }

    /* Does command line end with & */ 
    if (! strcmp(argv1[argc1 - 1], "&")) {
        amper = 1; // Set the amper flag to 1 to indicate background execution
        argv1[argc1 - 1] = NULL; // Remove "&" from the argument list
        }
    else 
        amper = 0; // Set the amper flag to 0 if "&" is not found 

    /* chanching the prompt */ 
    if(argc1 > 2 && ! strcmp(argv1[argc1 - 2], "=") && ! strcmp(argv1[argc1 - 3], "prompt")) {
        strcpy(prompt, argv1[argc1 - 1]); // Copy the new prompt value into the prompt variable
        continue;
    }

    /* read command */ 
    if(! strcmp(argv1[0], "read")) {
        strcpy(variables[count_var%20], "$");
        strcat(variables[count_var%20],argv1[1]); // Append the variable name
        fgets(var, 1024, stdin); 

        var[strlen(var) - 1] = '\0';

        strcpy(variables_val[count_var%20], var); // Copy the input value into the corresponding value slot
        count_var++;
        continue;
    }

    /* add variables */ 
    if(argc1 > 1 && argv1[0][0] == '$' && ! strcmp(argv1[1], "=")) {
        strcpy(variables[count_var%20], argv1[0]); // Copy the variable name 
        strcpy(variables_val[count_var%20], argv1[2]); // Copy the variable value
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
                // Loop through the stored variables to find a match
                if(!strcmp(variables[i], argv1[1])) {
                    printf("%s\n", variables_val[i]);
                }
            }
        } else {
            //echo
            // If none of the above, just echo the arguments back 
            for (int j = 1; j < argc1; j++) {
                printf("%s ", argv1[j]);
            }
            printf("\n");
        }
        continue;
    }

    /* cd command */     
    if(! strcmp(argv1[0], "cd")) {
        chdir(argv1[1]); // Change the current working directory to the directory specified in argv1[1]
        continue;
    }
    
    /* chanching the stdout */ 
    if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">")) {
        redirect = 1; // Set flag for stdout redirection
        redirectAppend = 0;
        redirectError = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    
    /* chanching the stderror */ 
    } else if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], "2>")) { // Check if the command includes "2>" for stderr redirection
        redirectError = 1; // Set flag for stderr redirection
        redirect = 0;
        redirectAppend = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    /* do append for a file */ 
    } else if(argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">>")) { // Check if the command includes ">>" for appending to a file
        redirectAppend = 1; // Set flag for append redirection
        redirect = 0;
        redirectError = 0;
        argv1[argc1 - 2] = NULL;
        outfile = argv1[argc1 - 1];
    } else { // If no redirection operator is found
        // Reset flags
        redirectAppend = 0;
        redirectError = 0;
        redirect = 0;  
    }
    
    /* for commands not part of the shell command language */ 
    if (fork() == 0) { 
        /* redirection of IO ? */
        if (redirect || redirectAppend) {
            if(redirect) { // If simple redirection (>)
                fd = creat(outfile, 0660); 
            } else { // If append redirection (>>)
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
            }
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stdout is now redirected */
        }
        if (redirectError) { // Check if there is redirection for stderr (2>)
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
                execvp(argv1[0], argv1); // Execute the first command
            } 
            /* 2nd command component of command line */ 
            close(STDIN_FILENO);
            dup(fildes[0]);
            close(fildes[0]); 
            close(fildes[1]); 
            /* standard input now comes from pipe */ 
            execvp(argv2[0], argv2); // Execute the second command
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