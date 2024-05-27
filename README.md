MY SHELL:

Renana Turgeman 322998287
Avia Oren 322233301

To run the program:
Run "make" to compile the program and then execute it with "./myshell".

The test.txt file contains examples of commands used for testing.

Our assumptions
1. in the arrow navigation: first navigate by the arrows, press enter and then the command will be executed.
^[[A => command back
^[[B => command up
for example: ^[[A^[[A^[[A => 3 commands back.
you can access just to the last 20 commands!

2. in the variable command you can access just to the last 20 variables.
if there is more then one value for a variable, the command echo $variable will present
all of them.

3. the size of a command and a variable is up to 1024. every command can have up to 10 arguments. the pipe command have no limited number of arguments.

4. in the read command, echo command when you ask for value for a variable and the variable command: you can use these commands just for one variable.

