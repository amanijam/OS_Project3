#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct memory_struct
{
	char *var;
	char *value;
};

struct memory_struct frameStore[1000];
struct memory_struct varStore[1000];

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
	printf("Initializing Variable Store with size %d\n", VARMEMSIZE);
	for (i = 0; i < VARMEMSIZE; i++)
	{
		varStore[i].var = "none";
		varStore[i].value = "none";
	}
}

// Set key value pair
int mem_set_value(char *var_in, char *value_in)
{
	int i;
	for (i = 0; i < VARMEMSIZE; i++)
	{
		if (strcmp(varStore[i].var, var_in) == 0)
		{
			varStore[i].value = strdup(value_in);
			return i;
		}
	}

	// Value does not exist, need to find a free spot.
	for (i = 0; i < VARMEMSIZE; i++)
	{
		if (strcmp(varStore[i].var, "none") == 0)
		{
			varStore[i].var = strdup(var_in);
			varStore[i].value = strdup(value_in);
			return i;
		}
	}

	return 1001;
}

// get value based on input key
char *mem_get_value(char *var_in)
{
	int i;
	for (i = 0; i < VARMEMSIZE; i++)
	{
		if (strcmp(varStore[i].var, var_in) == 0)
		{
			return strdup(varStore[i].value);
		}
	}
	return "Variable does not exist";
}



// Frame Store functions

void framestr_init()
{
	int i;
	printf("Initializing Frame Store with size %d\n", FRAMESIZE);
	for (i = 0; i < FRAMESIZE; i++)
	{
		frameStore[i].var = "none";
		frameStore[i].value = "none";
	}
}

// Return position in memory array where the key value pair was placed in
int insert_framestr(char *var_in, char *value_in)
{
	int i;
	for (i = 0; i < FRAMESIZE; i++)
	{
		if (strcmp(frameStore[i].var, "none") == 0)
		{
			frameStore[i].var = strdup(var_in);
			frameStore[i].value = strdup(value_in);
			return i;
		}
	}

	return 1001;
}

char *mem_get_from_framestr(int i)
{
	if (i < FRAMESIZE)
		return frameStore[i].value;
	else
		return "Invalid position";
}

void mem_remove_from_framestr(int i)
{
	frameStore[i].var = "none";
	frameStore[i].value = "none";
}
