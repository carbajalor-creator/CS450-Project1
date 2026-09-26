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

void syserror(const char *);

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

int onePipe (char* comm1Args[], char*comm2Args[]) {
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

int main() {

    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char* line_words[MAX_LINE_WORDS + 1];
    // True when stdin is connected to a terminal
    int interactive = isatty(STDIN_FILENO);

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    
    char *cmd1[] = {"echo", "hello is this thing on", NULL};
    char *cmd2[] = {"cat", NULL};
    char *cmd3[] = {"cat", NULL};
    //to call my functions    
    onePipe(cmd1, cmd2);
    twoPipes(cmd1, cmd2, cmd3);

    while (1) {
        if (interactive) {
            printf("lobo> ");
            fflush(stdout);
        }
        if (fgets(line, MAX_LINE_CHARS, stdin) == NULL) {
            break;
        }

        int num_words = split_cmd_line(line, line_words);

        for (int i=0; i < num_words; i++) {
            printf("%s\n", line_words[i]);
        }
        //if one pipe detected call onePipe(cmd1, cmd2)
        //if two pipes detected call twoPipes(cmd1, cmd2, cmd3)
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
