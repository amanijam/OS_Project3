#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Struct for variable store entry
struct memory_struct
{
	char *var;
	char *value;
};

// Struct for frame store entry
typedef struct frame_struct
{
	int pid;
	int pageNum;
	char *value;
} FrameSlice;

// Node struct for LRU_Queue
typedef struct frame_node
{
    struct frame_node *prev;
    int frameNum;
    struct frame_node *next;
} FrameNode;

// Struct for LRU_Queue
typedef struct Queue
{
	int totNumFrames;
	int count;
	FrameNode *head;
	FrameNode *tail;
} LRU_Queue;

// Struct for LRU_Hash
typedef struct Hash
{
	int size;
	FrameNode **array;
} LRU_Hash;

struct memory_struct varStore[1000];
FrameSlice *frameStore[1000];
LRU_Queue *lru_queue;
LRU_Hash *lru_hash;
int pageSize = 3;

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

void resetmem()
{
	for (int i = 0; i < VARMEMSIZE; i++)
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

void lru_queue_init(int numFrames){
	lru_queue = malloc(sizeof(LRU_Queue));
	lru_queue->totNumFrames = numFrames;
	lru_queue->count = 0;
	lru_queue->head = NULL;
	lru_queue->tail = NULL;
}

void lru_hash_init(int size){
	lru_hash = malloc(sizeof(LRU_Hash));
	lru_hash->size = size;
	lru_hash->array = malloc(lru_hash->size * sizeof(FrameNode));
	for(int i = 0; i < size; i++){
		lru_hash->array[i] = NULL;
	}
}

void framestr_init()
{
	for (int i = 0; i < FRAMESIZE; i++)
	{
		frameStore[i] = malloc(sizeof(FrameSlice));
		frameStore[i]->pid = -1;
		frameStore[i]->pageNum = -1;
		frameStore[i]->value = "none";
	}
	lru_queue_init(FRAMESIZE/pageSize);
	lru_hash_init(FRAMESIZE/pageSize);
}


void freeFrameStr()
{
	for (int i = 0; i < FRAMESIZE; i++) free(frameStore[i]);
}

// Remove FrameNode at tail of LRU queue
void dequeueLRU()
{
	if(lru_queue->tail == NULL) return;

	FrameNode *ogtail = lru_queue->tail;

	if(lru_queue->head == lru_queue->tail)
	{
		lru_queue->head = NULL;
		lru_queue->tail = NULL;
	}
	else
	{
		FrameNode *prev = ogtail->prev;
		prev->next = NULL;
		lru_queue->tail = prev;
	}
	
	free(ogtail);
	lru_queue->count--;
}

// Add new Frame Node to the head of the LRU_Queue with frame number fNum
void enqueueLRU(int fNum)
{
	if(lru_queue->count == lru_queue->totNumFrames)
	{
		lru_hash->array[lru_queue->tail->frameNum] = NULL;
		dequeueLRU();
	}
	FrameNode *new = malloc(sizeof(FrameNode));
	new->frameNum = fNum;

	if(lru_queue->head == NULL)
	{
		new->prev = NULL;
		new->next = NULL;
		lru_queue->head = new;
		lru_queue->tail = new;
	}
	else
	{
		new->prev = NULL;
		new->next = lru_queue->head;
		lru_queue->head->prev = new;
		lru_queue->head = new;
	}

	lru_hash->array[fNum] = new;
	lru_queue->count++;
}

// Add Frame Node with frame number fNum to front of queue
void referTo(int fNum){
	FrameNode *refFrame = lru_hash->array[fNum];
	
	// If it's not in the queue, use enqueue add to it to the head
	if(refFrame == NULL)
		enqueueLRU(fNum);
	// else if it's in the queue but not the head, detatch it from the middle of the queue and add it to the front
	else if(refFrame != lru_queue->head)
	{
		FrameNode *prev = refFrame->prev;
		if(refFrame == lru_queue->tail){
			prev->next = NULL;
			lru_queue->tail = prev;
		}
		else{
			FrameNode *next = refFrame->next;
			prev->next = next;
			next->prev = prev;
		}
		refFrame->next = lru_queue->head;
		refFrame->prev = NULL;
		lru_queue->head->prev = refFrame;
		lru_queue->head = refFrame;
	}
}

int getLRUFrameNum()
{
	if(lru_queue->tail == NULL) return -1;
	return lru_queue->tail->frameNum;
}

void evictFrame()
{
	dequeueLRU();
}

// Insert in frame store in first available spot
int insert_framestr(int pid, int pn, char *line)
{
	for (int i = 0; i < FRAMESIZE; i++)
	{
		if (strcmp(frameStore[i]->value, "none") == 0)
		{
			frameStore[i]->pid = pid;
			frameStore[i]->pageNum = pn;
			frameStore[i]->value = strdup(line);
			referTo(i/pageSize);
			return i;
		}
	}
	return 1001;
}

FrameSlice *mem_get_from_framestr(int i)
{
	referTo(i/pageSize);
	return frameStore[i];
}

FrameSlice *mem_read_from_framestr(int i)
{
	return frameStore[i];
}

void mem_remove_from_framestr(int i)
{
	frameStore[i]->pid = -1;
	frameStore[i]->pageNum = -1;
	frameStore[i]->value = "none";
}
