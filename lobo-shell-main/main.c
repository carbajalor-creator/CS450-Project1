#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "constants.h"
#include "parsetools.h"

//Redirect standard input to a file
// this handles < filename
void redirect_input(char* filename) {

    //file descriptor
    int fd;

    //open the file only for reading
    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        //print error message 
        perror("open");
        exit(1);
    }

    //make stdin come from this file
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        exit(1);
    }

    //we don't need the original file descriptor
   //file is already connected to stdin and dup2 succeeded
   if (close(fd)== -1) {
      perror("close");
      exit(1);
   }
}

//redirect standard output to a file
//append = 0 means >
//append = 1 means >>
void redirect_output(char* filename, int append) {
    int fd;

    if (append == 1) {
      // >> adds new output to the end of the file
      //open the file for writing only
      //create the file if it doesn't already exist
      //0666; permission setting used when the file has to be created
      fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    }

    else {
        // > replaces the old contents of the file
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }

    if (fd == -1) {
        perror("open");
        exit(1);
    }

    //make stdout go to this file
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        exit(1);
    }

    //don't need original file descriptor anymore
    if(close(fd) == -1) {
        perror("close");
        exit(1);
    }
}

//looks through the words of a command
//it handles: < filename, > filename, >> filename
//redirection words aren't placed in command_args
//regular command words are placed in command_args for execvp()
void setup_redirection(char** words, int num_words, char** command_args) {
    int i;
    int arg_index = 0;

    for (i = 0; i < num_words; i++) {

        //Milestone 6: normal output redirection >
        if (strcmp(words[i], ">") == 0) {
            redirect_output(words[i + 1], 0);

            //skip the filename
            i++;
        }

        //Milestone 7: append output redirection >>
        else if (strcmp(words[i], ">>") == 0) {
            redirect_output(words[i + 1], 1);

            //skip the filename
            i++;
        }

        //Milestone 8: input redirection <
        else if (strcmp(words[i], "<") == 0) {
            redirect_input(words[i + 1]);

            //skip the filename
            i++;
        }

        else {
            //save normal command words for execvp()
           command_args[arg_index] = words[i];
           arg_index++;
        }

    }

    //execvp() needs NULL at the end
    command_args[arg_index] = NULL;
}



int main() {

    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char* line_words[MAX_LINE_WORDS + 1];
    // True when stdin is connected to a terminal
    int interactive = isatty(STDIN_FILENO);

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while (1) {
        if (interactive) {
            printf("lobo> ");
            fflush(stdout);
        }
        if (fgets(line, MAX_LINE_CHARS, stdin) == NULL) {
            break;
        }

        int num_words = split_cmd_line(line, line_words);

//not using because I don't need to keeping printing every word
//made temporary execution code 
//
for (int i=0; i < num_words; i++) {
            printf("%s\n", line_words[i]);
        }
//
        if(num_words == 0) {
            continue;
        }  

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
        }

        else if (pid == 0) {
            //this array will hold the command without > and the filename
            char* command_args[MAX_LINE_WORDS + 1];

            //check for redirection
            setup_redirection(line_words, num_words, command_args);

            //run command
            execvp(command_args[0], command_args);

            //only will get here if evecvp fails
            perror("execvp");
            exit(1);
        }

        else {
            //parent waits for child
            if (wait(NULL) == -1) {
                perror("wait");
            }
        }

    }

    return 0;
}


