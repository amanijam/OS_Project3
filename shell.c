
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"
#include "shellmemory.h"

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[])
{
	printf("%s\n", "Shell version 1.1 Created January 2022");
	help();

	int error = system("mkdir backingStore");

	if (error == -1)
	{
		system("rm -rf backingStore");
		system("mkdir backingStore");
	}

	char prompt = '$';				// Shell prompt
	char userInput[MAX_USER_INPUT]; // user's input stored here
	int errorCode = 0;				// zero means no error, default

	// init user input
	for (int i = 0; i < MAX_USER_INPUT; i++)
		userInput[i] = '\0';

	// init shell memory
	mem_init();

	while (1)
	{
		printf("%c ", prompt);
		fgets(userInput, MAX_USER_INPUT - 1, stdin);
		if (feof(stdin))
			freopen("/dev/tty", "r", stdin); // switch to interactive mode after batch mode

		errorCode = parseInput(userInput);
		if (errorCode == -1)
			exit(99); // ignore all other errors
		memset(userInput, 0, sizeof(userInput));
	}

	return 0;
}

/* 	Extract words from the input then call interpreter
	For a chain of commands (delimited by semicolons (;)),
		send one command at a time to the interpreter.
	ASSUMPTION ABOUT ONE-LINERS: A semicolon at the end of
		any word marks the end of an instruction
*/
int parseInput(char ui[])
{
	char tmp[200];
	char *words[100];
	int a = 0;
	int b;
	int w = 0; // wordID
	int error;

	for (a = a; ui[a] == ' ' && a < 1000; a++)
		; // skip white spaces (at the front)

	while (ui[a] != '\0' && ui[a] != '\n' && a < 1000)
	{
		for (b = 0; ui[a] != '\0' && ui[a] != '\n' && ui[a] != ' ' && ui[a] != ';' && a < 1000; a++, b++)
			tmp[b] = ui[a]; // extract a word

		tmp[b] = '\0';

		words[w] = strdup(tmp);

		w++;

		if (ui[a] == '\0')
		{
			break;
		}
		else if (ui[a] == ';')
		{
			error = interpreter(words, w);

			if (error == -1)
				return error;

			a++;
			w = 0;
			for (a = a; ui[a] == ' ' && a < 1000; a++)
				; // skip white spaces
			continue;
		}
		a++;
	}

	error = interpreter(words, w);

	return error;
}
