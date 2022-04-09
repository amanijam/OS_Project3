#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct memory_struct
{
	char *var;
	char *value;
};

int framesize=FRAMESIZE;
int varmemsize=VARMEMSIZE;

struct memory_struct shellmemory[FRAMESIZE];
struct memory_struct varStore[VARMEMSIZE];

// Helper functions
int match(char *model, char *var)
{
	int i, len = strlen(var), matchCount = 0;
	for (i = 0; i < len; i++)
		if (*(model + i) == *(var + i))
			matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model)
{
	char token = '='; // look for this to find value
	char value[1000]; // stores the extract value
	int i, j, len = strlen(model);
	for (i = 0; i < len && *(model + i) != token; i++)
		; // loop till we get there
	// extract the value
	for (i = i + 1, j = 0; i < len; i++, j++)
		value[j] = *(model + i);
	value[j] = '\0';
	return strdup(value);
}

// Variable Store functions

void varStore_init()
{
	int i;
	for (i = 0; i < 1000; i++)
	{
		varStore[i].var = "none";
		varStore[i].value = "none";
	}
}

// Shell memory functions (Frame Store)

void mem_init()
{
	int i;
	for (i = 0; i < 1000; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
int mem_set_value(char *var_in, char *value_in)
{

	int i;

	for (i = 0; i < 1000; i++)
	{
		if (strcmp(shellmemory[i].var, var_in) == 0)
		{
			shellmemory[i].value = strdup(value_in);
			return i;
		}
	}

	// Value does not exist, need to find a free spot.
	for (i = 0; i < 1000; i++)
	{
		if (strcmp(shellmemory[i].var, "none") == 0)
		{
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return i;
		}
	}

	return 1001;
}

// Return position in memory array where the key value pair was placed in
int insert(char *var_in, char *value_in)
{
	int i;
	for (i = 0; i < 1000; i++)
	{
		if (strcmp(shellmemory[i].var, "none") == 0)
		{
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return i;
		}
	}

	return 1001;
}

// get value based on input key
char *mem_get_value(char *var_in)
{
	int i;

	for (i = 0; i < 1000; i++)
	{
		if (strcmp(shellmemory[i].var, var_in) == 0)
		{

			return strdup(shellmemory[i].value);
		}
	}
	return "Variable does not exist";
}

char *mem_get_value_from_position(int i)
{
	if (i < 1000)
	{
		return shellmemory[i].value;
	}
	else
		return "Invalid position";
}

void mem_remove_by_position(int i)
{
	shellmemory[i].var = "none";
	shellmemory[i].value = "none";
}
