# Shell
 Mav shell (msh), similar to bourne shell (bash), c-shell (csh), or korn shell (ksh). It will accept commands, fork a child process and execute those commands. The shell, like csh or bash, will run and accept commands until the user exits the shell.
 
# How to run:
 To run the program type the following commands :
 
    make
    ./msh
 
# Commands Supported:
 The shell supports all commands in /bin, /usr/bin/ and /usr/local/bin/. Other than that the shell supports the following commands
  ## listpids
    This command will display the pids of the last 15 children.
  ## history
    This command will display the last 50 commands typed by the user
     Typing !n, where n is a number between 1 and 50 will
     result in the shell re-running the nth command. 
 

