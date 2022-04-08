#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"

int MAX_ARGS_SIZE = 7;

int help();
int quit();
int badcommand();
int badcommandSet();
int badcommandFileDoesNotExist();
int badcommandPolicy();
int badcommandSameName();
int set(char *var, char *value);
int print(char *var);
int run(char *script);
int ls();
int resetmem();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size)
{
	int i;

	if (args_size < 1)
	{
		return badcommand();
	}

	for (i = 0; i < args_size; i++)
	{ // strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help") == 0)
	{
		if (args_size != 1)
			return badcommand();
		return help();
	}
	else if (strcmp(command_args[0], "quit") == 0)
	{
		if (args_size != 1)
			return badcommand();
		return quit();
	}
	else if (strcmp(command_args[0], "set") == 0)
	{
		if (args_size > MAX_ARGS_SIZE)
		{
			return badcommandSet();
		}
		else if (args_size < 3)
		{
			return badcommand();
		}
		else
		{
			char *link = " ";
			char buffer[1000];
			strcpy(buffer, command_args[2]);
			for (int i = 3; i < args_size; i++)
			{
				strcat(buffer, link);
				strcat(buffer, command_args[i]);
			}

			return set(command_args[1], buffer);
		}
	}
	else if (strcmp(command_args[0], "echo") == 0)
	{
		if (args_size != 2)
			return badcommand();
		char buffer[100];
		strcpy(buffer, command_args[1]);
		if (buffer[0] == '$')
		{
			memmove(buffer, buffer + 1, strlen(buffer));
			return print(buffer);
		}
		else
		{
			printf("%s\n", buffer);
		}
	}
	else if (strcmp(command_args[0], "print") == 0)
	{
		if (args_size != 2)
			return badcommand();
		return print(command_args[1]);
	}
	else if (strcmp(command_args[0], "run") == 0)
	{
		if (args_size != 2)
			return badcommand();
		char command[50] = "";
		strcat(command, "cp ");
		strcat(command, command_args[1]);
		strcat(command, " backingStore");
		system(command);
		return run(command_args[1]);
	}
	else if (strcmp(command_args[0], "my_ls") == 0)
	{
		if (args_size != 1)
			return badcommand();
		return ls();
	}
	else if (strcmp(command_args[0], "exec") == 0)
	{
		if (args_size > 5)
		{
			return badcommandSet();
		}
		else if (args_size < 3)
		{
			return badcommand();
		}

		char *policy = command_args[args_size - 1];
		int numOfProgs = args_size - 2;

		// array of script file names
		// load script to backing store
		char *scripts[3];
		for (int i = 1; i < numOfProgs + 1; i++)
		{
			scripts[i - 1] = command_args[i];
			char command[50] = "";
			strcat(command, "cp ");
			strcat(command, scripts[i - 1]);
			strcat(command, " backingStore");
			system(command);
		}

		if (strcmp(policy, "FCFS") == 0 || strcmp(policy, "SJF") == 0 || strcmp(policy, "RR") == 0 || strcmp(policy, "AGING") == 0)
		{
			setPolicy(policy);
			return schedulerStart(scripts, numOfProgs);
		}
		else
		{
			return badcommandPolicy();
		}
	}
	else
		return badcommand();
}

int help()
{

	char help_string[] = "COMMAND			DESCRIPTION\n \
	help		Displays all the commands\n \
	quit		Exits / terminates the shell with “Bye!”\n \
	set VAR STRING	Assigns a value to shell memory\n \
	print VAR	Displays the STRING assigned to VAR\n \
	echo VAR STRING	Displays the string on a new line \n \
	run SCRIPT.TXT	Executes the file SCRIPT.TXT\n \
	my_ls 		Lists all files present in current directory\n \
	(multiple commands)	Runs up to 5 commands per line, separated by\n \
				semicolons (;) added to the end of the last word in each command\n \
	exec prog1 prog2 prog3 POLICY	Executes up to 3 commands\n \
					according to the scheduling policy";
	printf("%s\n", help_string);
	return 0;
}

int quit()
{
	system("rmd -rf backingStore");
	printf("%s\n", "Bye!");
	exit(0);
}

int badcommand()
{
	printf("%s\n", "Unknown Command");
	return 1;
}

int badcommandSet()
{
	printf("%s\n", "Bad command: Too many tokens");
	return 2;
}

// For run command only
int badcommandFileDoesNotExist()
{
	printf("%s\n", "Bad command: File not found");
	return 3;
}

int badcommandPolicy()
{
	printf("%s\n", "Bad command: Invalid policy");
	return 4;
}

int badcommandSameName()
{
	printf("%s\n", "Bad command: same file name");
	return 5;
}

int set(char *var, char *value)
{
	mem_set_value(var, value);
	return 0;
}

int print(char *var)
{
	printf("%s\n", mem_get_value(var));
	return 0;
}

int run(char *script)
{
	int errCode = 0;
	char line[1000];
	char file[100] = "backingStore/";
	strcat(file, script);
	FILE *p = fopen(file, "rt");

	if (p == NULL)
		return badcommandFileDoesNotExist();

	// Load script source code into shellmemory
	int lineCount = 0;
	char lineBuffer[10];
	int startPosition; // contains position in memory of 1st line of code

	while (!feof(p) || lineCount > 2)
	{
		fgets(line, 999, p);
		lineCount++;
		sprintf(lineBuffer, "%d", lineCount);

		if (lineCount == 1)
			startPosition = set(lineBuffer, line);
		else
			set(lineBuffer, line);

		memset(line, 0, sizeof(line));
	}
	fclose(p);

	char *currCommand;
	int counter = 0;
	for (int i = 0; i < lineCount; i++)
	{
		currCommand = mem_get_value_from_position(startPosition + counter);
		counter++;				 // increment pc
		parseInput(currCommand); // from shell, which calls interpreter()
	}

	// remove script course code from shellmemory
	for (int i = startPosition; i < startPosition + lineCount; i++)
	{
		mem_remove_by_position(i);
	}

	return errCode;
}

int ls()
{
	return system("ls -1"); // lists directories in alphabetical order, 1 entry per line
}

int resetmem()
{
	varStore_init();
	return 0;
}
