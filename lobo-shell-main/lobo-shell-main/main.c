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


// arr = array of words that have been split up and stored in line_words[]
// index = num of words
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


void split_cmd(char *arr[], int index) {
	int p = pipehelper(arr, index);
	
	
	char temp[p + 1][100]; // array of p strings, holds up to 100 chars (including null bit)
	
	for (int i = 0; i < p + 1; i++) {
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
	

	for (int i = 0; i <= j; i++) {
		printf("%s\n", temp[i]);
	}

	//printf("%s\n", temp[0]);
	//printf("%s\n", temp[1]);

	
}


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
	// passing line_words into simplecommands(arr);
	// Will need a way to distinguish between simple commands 
	// and those with pipes.
	
	// if pipes = 0, then call simplecommands
	//
	int exp = pipehelper(line_words, num_words);
	switch (exp) {
		case 0: // if pipe == 0;
		simplecommands(line_words);
		break;
		case 1: {
		int count;
		char **result = strpipe(line_words, num_words, &count);

		for (int i = 0; i < count; i++) {
			printf("%s\n", result[i]);
		}

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}

		break;
			}
		case 2: {
	       	int count; 	
		
		char **result  = strpipe(line_words, num_words, &count);

		for (int i = 0; i < count; i++) { 
			printf("%s\n", result[i]);
		}

		for (int i = 0; i < count; i++) {
			free(result[i]);
		}


		//split_cmd(line_words, num_words);
		break; 
			}
		
	}

			}
	//simplecommands(line_words);
	//strpipe(line_words, num_words);	
	//printf("# of pipes: %d\n", pipehelper(line_words, num_words));
	
	
    return 0;
}


