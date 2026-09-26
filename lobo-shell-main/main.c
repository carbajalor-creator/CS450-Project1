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



// Void simplecommands() - forks and execs a command with no pipes or redirects
// 	char *arr[]: passed to execvp(), assumed to be simple commands;
// 	
// 	forks arr[] and execs arguements within arr[]. Can only 
// 	process simple commands
//
void simplecommands(char *arr[]) {
	pid_t pid;

	pid = fork();

	if (pid < 0) {
		printf("failed fork\n");
		exit(1);

	} else if (pid == 0) { //child process
		execvp(arr[0], arr);
		perror("if you see this..something has gone wrong.\n");
		
	} else { wait(NULL); } // parent process waits and reaps child

}



// int pipehelper() - returns the # of pipes in a given argument array
// 	char *arr[]: used to iterate through and check for pipes
// 	int index: holds the num of indexs for arr[]
int pipehelper(char *arr[], int index) {
	int p = 0;
	for (int i = 0; i < index; i++) {
		char *compare = strchr(arr[i], '|');

		if (compare != NULL) {
			p++;	
		} else { } // nothing happens
	}
	return p;
}

// char **strpipe() - appends strings before each pipe and  returns 
// 		      an array of strings
// 	char *arr[]: iterated through and appends strings to a temp array
// 	int index: holds the num of indexes for arr[]
// 	int *count: value that holds (num of pipes + 1) value, used to free
// 		    the array at a later point
char **strpipe(char *arr[], int index, int *count) {
	

	int p = pipehelper(arr, index);
	

	char **temp = malloc((p + 1) * sizeof(char *));
	
	for (int i = 0; i <= p; i++) {
		temp[i] = malloc(100);
		temp[i][0] = '\0';
	}
	
	int j = 0;

	for (int i = 0; i < index; i++) {

		char *compare = strchr(arr[i], '|');

		if (compare == NULL) {
			strcat(temp[j], arr[i]);
			strcat(temp[j], " ");
			
		} else { j++; }	

	}
	*count = p + 1;
	return temp;
}


void syserror(const char *);

int onePipe (char* comm1Args[], char* comm2Args[]) {
    int pfd[2];
    pid_t pid;

    if (pipe(pfd) == -1) { syserror("couldn't create pipe"); }
    
    pid = fork(); //first fork runs first command
    if (pid == -1) { syserror("first fork failed"); }
    else if (pid == 0) {
        if (close(1) == -1) { syserror("couldn't close stdout"); }
        if (dup(pfd[1]) == -1) {syserror("error duping write end"); }
        if (close(pfd[0]) == -1 || close(pfd[1]) == -1) { syserror("couldn't close pipes file descriptors"); }

        execvp(comm1Args[0], comm1Args);
        syserror("couldn't exececute first command");
    }

    pid = fork(); //second fork runs second command
    if (pid == -1) { syserror("first fork failed"); }
    else if (pid == 0) {
        if (close(0) == -1) { syserror("couldn't close stdin"); }
        if (dup(pfd[0]) == -1) { syserror("error duping read end"); }
        if (close(pfd[0]) == -1 || close(pfd[1]) == -1) { syserror("couldn't close second childs pfd"); }
        execvp(comm2Args[0], comm2Args);
        syserror("couldn't exec second command");
    }
    if (close(pfd[0]) == -1){ syserror("parent couldn't close pipe read end"); }
    if (close(pfd[1]) == -1) { syserror("parent couldn't close pipe write end"); }
    while (wait(NULL) != -1)
        ;
    return 0;
}

int twoPipes( char* comm1Args[], char* comm2Args[], char* comm3Args[]){ 
    int pfd1[2]; 
    int pfd2[2];
    pid_t pid; 

    if (pipe(pfd1) == -1){ syserror( "Could not create a pipe" ); }
    if (pipe(pfd2) == -1){ syserror("could not create a second pipe"); }

    //FIRST FORK
    pid = fork();

    if (pid == -1) { syserror("first fork failed"); }
    else if (pid == 0) {
        if (close(1) == -1) { syserror("couldn't close stdout"); }
        if (dup(pfd1[1]) == -1) { syserror("error duping pipe 1 write end"); }
        close(pfd1[0]);
        close(pfd1[1]);

        close(pfd2[0]);
        close(pfd2[1]);

        execvp(comm1Args[0], comm1Args);
        syserror("couldnt exec command 1");
    }

    //SECOND FORK
    pid = fork();

    if (pid == -1) { syserror("second fork failed"); }
    else if (pid == 0) { 
        if (close(0) == -1) { syserror("couldn't close stdin"); }
        if (dup(pfd1[0]) == -1) { syserror("error duping pipe 1 read end"); }
        if (close(1) == -1) { syserror("couldn't close stdout"); }
        if (dup(pfd2[1]) == -1) { syserror("error duping pipe 2 write end"); }
        close(pfd1[0]);
        close(pfd1[1]);
        close(pfd2[0]);
        close(pfd2[1]);
        execvp(comm2Args[0], comm2Args);
        syserror("couldn't exec command 2");
    }

    //THIRD FORK
    pid = fork();
    if (pid == -1) { syserror("third fork failed"); }
    else if (pid == 0) {
        if (close(0) == -1) { syserror("couldn't close stdin"); }
        if (dup(pfd2[0]) == -1) { syserror("error duping pipe 2s read end"); }
        close(pfd1[0]);
        close(pfd1[1]);
        close(pfd2[0]);
        close(pfd2[1]);

        execvp(comm3Args[0], comm3Args);
        syserror("couldn't exec command 3");
    }

close(pfd1[0]);
close(pfd1[1]);
close(pfd2[0]);
close(pfd2[1]);

while (wait(NULL) != -1)
    ;

    return 0;
}

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


// Void Redir: - sets up the setup_redirection
//	char *arr[]: passed to setup_redirection for redirection
// 	int index: holds the num of indexes for arr[]
void Redir(char *kit[], int index) {
	pid_t pid = fork();

	if (pid == -1) {
		perror("Fork failed\n");

	} else if (pid == 0) {
		char* command_args[MAX_LINE_WORDS + 1];

		setup_redirection(kit, index, command_args);

		execvp(command_args[0], command_args);

		perror("execvp failed :(\n");
		exit(1);
	} else {
		if (wait(NULL) == -1) { perror("wait\n"); }
	}
}

// int checkForRedir - returns 0 if there are no redirection symbols, 
// 		       returns 1 if ">, >>, <" is within a given array index
// 	char *arr[]: iterated through to look for redirection symbols
// 	int index: holds the num of indexes for arr[]

int checkForRedir(char *arr[], int num_words) {
	for (int i = 0; i < num_words; i++) {
		if (strcmp(arr[i], ">") == 0 || 
		    strcmp(arr[i], ">>") == 0 ||
		    strcmp(arr[i], "<") == 0) {
		    return 1;
		}
	}

	return 0;
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
	

	// >, >>, <
	//
	//
	
	int redir_val = checkForRedir(line_words, num_words);		
	
	int exp = pipehelper(line_words, num_words);

	
	if (exp == 0 && redir_val == 1) {
		//printf("There was a symbol\n");
		Redir(line_words, num_words);
	} else if (exp == 0) {
		simplecommands(line_words);
	} else if (exp == 1) {
		int count;
		char **result = strpipe(line_words, num_words, &count);
		
		//char str1[] = malloc(strlen(result[0]) * sizeof(char));
		
		char *cmd1[MAX_LINE_WORDS + 1];
		char *cmd2[MAX_LINE_WORDS + 1];

		split_cmd_line(result[0], cmd1);
		split_cmd_line(result[1], cmd2);

		onePipe(cmd1, cmd2);

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}
	
	} else if (exp == 2) {
		int count; 
		char **result = strpipe(line_words, num_words, &count);

		char *cmd1[MAX_LINE_WORDS + 1];
		char *cmd2[MAX_LINE_WORDS + 1];
		char *cmd3[MAX_LINE_WORDS + 1];	

		split_cmd_line(result[0], cmd1);
		split_cmd_line(result[1], cmd2);
		split_cmd_line(result[2], cmd3);

		twoPipes(cmd1, cmd2, cmd3);

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}
	}
	
	/*
	switch(exp) {
		case 0:
		simplecommands(line_words);
		break;
		case 1: {
		int count;
		char **result = strpipe(line_words, num_words, &count);
		
		//char str1[] = malloc(strlen(result[0]) * sizeof(char));
		
		char *cmd1[MAX_LINE_WORDS + 1];
		char *cmd2[MAX_LINE_WORDS + 1];

		split_cmd_line(result[0], cmd1);
		split_cmd_line(result[1], cmd2);

		onePipe(cmd1, cmd2);

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}
		break;
		}
		case 2: {
		int count; 
		char **result = strpipe(line_words, num_words, &count);

		char *cmd1[MAX_LINE_WORDS + 1];
		char *cmd2[MAX_LINE_WORDS + 1];
		char *cmd3[MAX_LINE_WORDS + 1];	

		split_cmd_line(result[0], cmd1);
		split_cmd_line(result[1], cmd2);
		split_cmd_line(result[2], cmd3);

		twoPipes(cmd1, cmd2, cmd3);

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}
		break;
		}
		
	} */
	
	// if all else fails, set up redirection
	
	/*	
	pid_t pid = fork();

	if (pid == -1) {
		perror("Fork failed\n");

	} else if (pid == 0) {
		char* command_args[MAX_LINE_WORDS + 1];

		setup_redirection(line_words, num_words, command_args);

		execvp(command_args[0], command_args);

		perror("execvp failed :(\n");
		exit(1);
	} else {
		if (wait(NULL) == -1) { perror("wait\n"); }
	}
	*/
   

    }

    return 0;
}

void syserror(const char *s) 
{
    extern int errno;
    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}


