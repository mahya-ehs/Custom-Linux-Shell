#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include<readline/history.h>
#include <sys/wait.h>

//-------colors---------
#define RED "\x1b[31m" 
#define GREEN "\x1b[32m" 
#define YELLOW "\x1b[33m" 
#define BLUE "\x1b[34m" 
#define MAGENTA "\x1b[35m" 
#define CYAN "\x1b[36m" 
#define RESET "\x1b[0m" 

static char* currentDirectory;
char** history;
int historyLength;
char* built_in_commands [] = {"cd", "history", "pwd", "exit", "clear", "help", "date"};

//------ executing commands with pipe ------
void exe_pipe_command(char** input, int num_of_commands)
{
    pid_t pid;
    int fd_odd[2];
    int fd_even[2];

    char** command = malloc(512 * sizeof(char*));
	int endOfCommands = 0;
    int command_num = 0;
    int j = 0;
    int k = 0;
    while (input[j] != NULL && endOfCommands != 1)
    {
        k = 0;
        while (strcmp(input[j], "|") != 0)
        {
			command[k] = input[j];
			j++;	
			if (input[j] == NULL)
            {
				endOfCommands = 1;
				k++;
				break;
			}
			k++;
		}
        command[k] = NULL;
        j++;
        if (command_num % 2 != 0)
        {
            if (pipe(fd_odd) == -1)
                return;
        } 
        else
        {
            if (pipe(fd_even) == -1)
                return;
        }
        
        pid = fork();
        if(pid == -1)
        {			
            //last command
            if (command_num != num_of_commands - 1)
            {
                if (command_num % 2 != 0)
                {
                    close(fd_odd[1]); // for odd i
                }
                else
                {
                    close(fd_even[1]); // for even i
                } 
            }			
            fprintf(stderr, RED"Child process could not be created\n"RESET);
            return;
        }

        //child
        if(pid == 0)
        {
            // first command
            if (command_num == 0)
            {
                //fd2[1] -> write
                dup2(fd_even[1], STDOUT_FILENO);
            }
            //last command
            else if (command_num == num_of_commands - 1)
            {
                if (num_of_commands % 2 != 0)
                {
                    dup2(fd_odd[0],STDIN_FILENO);
                }
                else
                { 
                    dup2(fd_even[0],STDIN_FILENO);
                }
            }

            //middle command
            else
            {
                if (command_num % 2 != 0)
                {
                    dup2(fd_even[0],STDIN_FILENO); 
                    dup2(fd_odd[1],STDOUT_FILENO);
                }
                else
                { 
                    dup2(fd_odd[0],STDIN_FILENO); 
                    dup2(fd_even[1],STDOUT_FILENO);					
                } 
            }
            if (execvp(command[0], command) == -1)
            {
                printf(RED "There was a problem executing the command\n"RESET);
                kill(getpid(),SIGTERM);
            }		
        }
                
        // closing description files on parent
        if (command_num == 0)
        {
            close(fd_even[1]);
        }
        else if (command_num == num_of_commands - 1)
        {
            if (num_of_commands % 2 != 0)
            {					
                close(fd_odd[0]);
            }
            else
            {					
                close(fd_even[0]);
            }
        }
        else
        {
            if (command_num % 2 != 0)
            {					
                close(fd_even[0]);
                close(fd_odd[1]);
            }
            else
            {					
                close(fd_odd[0]);
                close(fd_even[1]);
            }
        }	
        waitpid(pid, NULL, 0);
        command_num++;	
    }
}

//------ executing usual linux commands ------
void exe_command(char** command)
{
    // Forking a child
	pid_t pid = fork();

    //error
	if (pid == -1) 
    {
		fprintf(stderr, RED"\nFailed forking child\n"RESET);
		return;
	}

    //child process
	else if (pid == 0) 
	{
		if (execvp(command[0], command) == -1) 
		{
			fprintf(stderr, RED"\nCould not execute command\n"RESET);
		}
		exit(0);
	}

    //parent process
	else 
	{
		// waiting for child to terminate
		wait(NULL);
		return;
	}
}

//------- executing built in commands ------
void exe_builtin_command(char** command)
{
    if (strcmp(command[0], "exit") == 0)
    {
        printf(YELLOW"\n\t======================================\n");
        printf("\t            See you later\n");
        printf("\t======================================\n" RESET);
        printf("\n");
        exit(0);
    }

    if (strcmp(command[0], "cd") == 0)
    {
        if (command[1] == NULL || strcmp(command[1], "~") == 0)
		    chdir(getenv("HOME")); 
	    
	    else
        { 
		    if (chdir(command[1]) == -1)
			    fprintf(stderr, RED "%s: no such directory\n" RESET, command[1]);
            else
                chdir(command[1]);
		}
    }

    if (strcmp(command[0], "history") == 0)
    {
        for (int i = 0; i < historyLength; i++)
            printf("%s\n",history[i]);
        
    }

    if (strcmp(command[0], "pwd") == 0)
    {
        fprintf(stdout, "%s\n", getcwd(currentDirectory, 1024));   
    }

    if (strcmp(command[0], "help") == 0)
    {
        puts(YELLOW"\n\t========== HELP SECTION =========="RESET
		BLUE"\n\tList of Commands supported:"
		"\n\t>cd"
		"\n\t>pwd"
        "\n\t>exit"
        "\n\t>date"
        "\n\t>history"
		"\n\t>all other general commands available in UNIX shell"
		"\n\t>pipe handling (multi pipe is allowed)"RESET
        YELLOW"\n\t=================================="RESET);
    }

    if (strcmp(command[0], "clear") == 0)
    {
        system("clear");
    }

    if (strcmp(command[0], "date") == 0)
    {
        time_t t;  
        time(&t);
        fprintf(stdout, "today is: " YELLOW "%s" RESET, ctime(&t));
    }
}

void addToHistory(char* cmd)
{
    history[historyLength] = strdup(cmd);

    //I had to comment this part because it didn't work completely right

    // FILE* fptr;
    // fptr = fopen("history.txt", "a");
    // fprintf(fptr, "%s", cmd);
    // fputc('\n', fptr);
    // fclose(fptr); 

    historyLength ++;
}

//------check whether it's builtin or not------
int command_type(char** command)
{
    if (command[0] == NULL)
        return 0;

    for (int i = 0; i < 7; i++)
    {
        //a none-piped builtin command
        if (strcmp(command[0], built_in_commands[i]) == 0)
            return 1;   
    }
    int j = 0;
    while (command[j] != NULL)
    {
        //a piped command
        if (strcmp(command[j], "|") == 0)
            return 2;
        j++;
    }
    //a non-piped command
    return 3;
}

//------parsing the input and removing spaces------
char** parse_command(char* input)
{
    int init_size = strlen(input);
    int buffer_size = 64;
    int i = 0;

	char *input_string = strtok(input, " \t\n\a\r");
    char **array = malloc(buffer_size * sizeof(char*));

	while (input_string != NULL)
	{
        array[i] = input_string;
        i++;
		input_string = strtok(NULL, " \t\n\a\r");
	}

    return array;
}

//------take the input from user------
char* get_input(char* cmd)
{
	fgets(cmd, 512, stdin);
	if ((strlen(cmd) > 0) && 
    ((cmd[strlen (cmd) - 1] == '\n') || ((cmd[strlen (cmd) - 1] == ' '))))
        	cmd[strlen (cmd) - 1] = '\0';
    return cmd;
}

//------showing the prompot------
void load_prompt()
{
    char cwd[255];
    char hostn[1204] = "";
	gethostname(hostn, sizeof(hostn));

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf(CYAN "%s@%s:" GREEN "%s > " RESET,getenv("LOGNAME"), hostn, cwd);
    
}

//------ handle ctrl+c -------
void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    fprintf(stderr, RED "\nCannot be terminated using Ctrl+C \n" RESET);
    fflush(stdout);
}

int main()
{
    char* input;
    char** command;
    char cmd[512];
    historyLength = 0;
    history = malloc(1024 * sizeof(char*));

    system("clear");
    signal(SIGINT, sigintHandler);
    
    while(1)
    {
        load_prompt();
        input = get_input(cmd);
        char* tmp = strdup(input);
        if((strtok(tmp," \n\t")) != NULL)
        {
            addToHistory(input);
            command = parse_command(input);
            //built in command
            if (command_type(command) == 1)
            {                
                exe_builtin_command(command);
            }

            //pipe commands
            else if (command_type(command) == 2)
            {
                //counting the pipes (or commands)
                int j = 0, num_of_pipes = 1;
                while (command[j] != NULL)
                {
                    if (strcmp(command[j], "|") == 0)
                    {
                        num_of_pipes ++;
                    }
                    j++;
                }
                exe_pipe_command(command, num_of_pipes);
            }

            //linux commands
            else if (command_type(command) == 3)
            {                
                exe_command(command);
            }
        }   
    }
}
