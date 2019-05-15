/*
  Pranav Bhandari
  1001551132
*/

// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments



/*
  The program is supposed to ignore SIGINT and SIGSTP signals only for 
  the children of the main process.
*/

//Handler made to catch SIGCHLD signals
void sigHandler(int signum)
{
  int status;
  switch(signum)
  {
    case SIGCHLD: // note that the last argument is important for the wait to work
                  waitpid(-1, &status, WNOHANG);
                  break;
  }
}

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  //a 2d array of characters to maintain all the commands 
  char **history  = (char**)malloc(50*sizeof(char*));
  int i;
  for(i=0;i<50;i++)
    history[i] = (char*)malloc(MAX_COMMAND_SIZE);
  //index to maintain history
  int index = 0;
  //index to maintain list of pids
  int index2 = 0;
  //array of pids
  pid_t array_of_pids[20];

  //making a sigset mask to add all the signals to block/unblock
  sigset_t mask;
  //emptying the set before adding signals to block
  sigemptyset(&mask);
  //Adding the signals to the sigset
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGTSTP);

  //making a handler for the signal SIGCHLD
  struct sigaction act;
  memset (&act, '\0',sizeof(act));
  act.sa_handler = &sigHandler;

  if(sigaction(SIGCHLD, &act, NULL)<0)
  {
    perror("Sigaction: Error");
    return 1;
  }
  
  while( 1 )
  {
    //This code blocks the signals in sigset mask from affecting the code
    sigprocmask(SIG_BLOCK,&mask,NULL);
    // Print out the msh prompt
    printf ("msh> ");

    /* 
       Read the command from the commandline.  The
       maximum command that will be read is MAX_COMMAND_SIZE
       This while command will wait here until the user
       inputs something since fgets returns NULL when there
       is no input
    */
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */

    /*
      Checking if the command entered starts from '!' and if the last command was history,
      Typing !n, where n is a number between 1 and 50 will
      result in the shell re-running the nth command. If the nth command does not exist then
      your shell will state “Command not in history.
    */
    char c = cmd_str[0];
    if(index!=0)
      if(c == '!' && (strcmp(history[index-1],"history\n")==0))
      {
        int command = (int)cmd_str[1];
        if((command-48)>index)
        {
          printf("Command not in history.\n");
          continue;
        }
        strcpy(cmd_str,history[command-48]);
      }

    if(strcmp(cmd_str,"exit\n")==0 || strcmp(cmd_str,"quit\n")==0)
        exit(0);

    char *token[MAX_NUM_ARGUMENTS];
    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                                                                               
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;
  
    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }


    //My c code
    //In case nothing is entered the loop goes to the next iteration
    if(token[0]==NULL)
      continue;

    //if cd is typed as the command
    else if(strcmp(token[0],"cd")==0)
    {
        if(chdir(token[1]) !=0)
          perror("chdir() failed: ");
    }

    //listing all the previous commands (upto 50) if history was entered
    else if(strcmp(token[0],"history")==0)
    {
      for(i=0;i<index;i++)
        printf("%d: %s",i,history[i]);
    }

    //listing all pids (upto 15) 
    else if(strcmp(token[0],"listpids")==0)
    {
      for(i=0;i<index2;i++)
      {
        printf("%d: %i\n",i,array_of_pids[i]);
      }
    }

    /*
      when bg is pressed then SIGCONT signal is passed to
      all the pids from the last till first till one of them recieves the signal
      and kill() executes successfully. 
    */
    else if(strcmp(token[0],"bg")==0)
    {
      int temporary =0;
      for(i=index2-1;i>=0;i--)
      {
        if(kill(array_of_pids[i],SIGCONT)==0)
        {
          temporary =-1;
          break;
        }
      }
      //if no process was backgrounded then print out error
      if(temporary == 0)
        printf("bg : no process to background\n");
    }

    /*
      if any other command is entered the program forks and
      creates a child and tries to exec in 3 locations: 
       "/bin" , "/usr/bin" and "/usr/local/bin".
      If exec fails all 3 times the program prints command not found
      and exits from the child.
    */
    else
    { 
      array_of_pids[index2] = fork();
      int status;
      char command[50];
      strcpy(command,"/bin/");
      strcat(command,token[0]); 
      if(array_of_pids[index2] == 0)  //child process
      {
        //unblocking the signals in the childprocess
        sigprocmask(SIG_UNBLOCK,&mask,NULL);
        //trying to exec and find the commmand in location "/bin"
        execvp(command,token);
        //if exec fails try again in different directory
        //trying to exec and find the commmand in location "/usr/bin"
        strcpy(command,"/usr/bin/");
        strcat(command,token[0]);
        execvp(command,token);
        //if exec fails again try in another directory
        //trying to exec and find the commmand in location "/usr/local/bin"
        strcpy(command,"/usr/local/bin/");
        strcat(command,token[0]);
        execvp(command,token);
        //exec failed in all 3 locations therefore printing that the command is not found.
        printf("%s : Command not found\n",token[0]);
        exit(0);
      }
      else 
      {
        //blocking the signal in the main process 
        sigprocmask(SIG_BLOCK,&mask,NULL);
        pause();
      }
      /*
        if the number of pids reaches 15
        Instead of incrementing index2 I move all the pids up one index
        and save the next pid in the 15th index.
        The pid saved in the first index is lost.
        This continues everytime the program forks after 15 times.
      */
      if(index2 == 15)
        for(i = 1; i< 16;i++)
        {
          array_of_pids[i-1] = array_of_pids[i];
        }
      else
        index2++; 
    }
    
    //storing the commands in the 2d array history only if the command was not empty
    if(!(strcmp(cmd_str,"")==0))
    {
      strcpy(history[index],cmd_str);
      /*
        if the number of commands reaches 50
        Instead of incrementing index I move all the commands up one index
        and save the command in the 50th index.
        This continues everytime a command is entered after 50 commands.
      */
      if(index == 50)
        for(i = 1; i< 50;i++)
          {
            history[i-1] = history[i];
          }
      else
        index++;
    }

    free( working_root );
  }

  return 0;
}
