#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"

typedef struct PCB
{
    int pid;
    int startMem;
    int len; // length = number of lines of code
    int lenScore;
    int pc; // current line to execute
    int pageTable[100];
    char *scriptName;
    struct PCB *next;
} PCB;

typedef struct frame_struct
{
	int pid;
	int pageNum;
	char *value;
} FrameSlice;

PCB *head = NULL; // global head of ready queue
PCB *progQueueHead = NULL;
char *policy;
int latestPid = 0; // holds last used pid, in order to ensure all pid's are unique
//int pageSize = 3; // size of page in num of lines of code
int evict = 0;

void setPolicy(char *p);
int schedulerStart(char *scripts[], int progNum);
void copyContents(PCB *prog, PCB *ready);
void freeProgQueue(int progNum);
void runQueue(int progNum);
int loadPage(PCB *pcb);
bool age();
void moveToBackOfQueue(int pid);
int enqueue(int start, int len, int pageTable[], char *name);
int prepend(int start, int len, int pageTable[], char *name);
int insertInQueue(int start, int len, int pageTable[], char *name);
int dequeue();
void removeFromQueue(int pid);

void setPolicy(char *p)
{
    policy = p;
}

int schedulerStart(char *scripts[], int progNum)
{
    latestPid = -1; // initialized latestPid

    char line[1000];
    char emptyLine[] = "EMPTY";
    int lineCount, startPosition, position;

    // Only load the first 2 pages of each program into the frame store
    for (int i = 0; i < progNum; i++)
    {
        char file[100] = "backingStore/";
        strcat(file, scripts[i]);
        FILE *p = fopen(file, "rt");
        if (p == NULL)
            return badcommandFileDoesNotExist();

        // initialize page table
        int curPageTable[100]; 
        for(int j = 0; j < 100; j++) curPageTable[j] = -1;

        lineCount = 0;
        startPosition; // contains position in memory of 1st line of code
        position; // index in frame store

        // Loading first 2 pages 
        int pt_indx = 0; // page table index
        int pageCount;
        int counter; // count line per page
        for(pageCount = 0; pageCount < 2; pageCount++){
            counter = 0;
            if(feof(p)) break; // if the file is empty, don't insert an empty page into the frame store

            // Insert one page of size pageSize
            while(!feof(p) && counter < pageSize)
            {
                fgets(line, 999, p);
                lineCount++;
                
                if (lineCount == 1)
                {
                    startPosition = insert_framestr(latestPid + 1, pt_indx, line);
                    position = startPosition;
                }
                else
                    position = insert_framestr(latestPid + 1, pt_indx, line);

                // update page table
                if (position % pageSize == 0)
                {
                    curPageTable[pt_indx] = position / pageSize;
                    pt_indx++;
                }

                memset(line, 0, sizeof(line));
                counter++;
            }

            // Correct 
            while (counter % pageSize != 0)
            {
                insert_framestr(latestPid + 1, pt_indx, emptyLine);
                counter++;
            }
        }

        // Continue counting the number of lines
        while(!feof(p)){
            fgets(line, 999, p);
            lineCount++;
        }

        fclose(p);

        // Add to ready queue
        if (strcmp(policy, "SJF") == 0 || strcmp(policy, "AGING") == 0)
            insertInQueue(startPosition, lineCount, curPageTable, scripts[i]); // add PCB to an ordered queue in inc order by program length (lines)
        else
            enqueue(startPosition, lineCount, curPageTable, scripts[i]); // add PCB to the end of queue (no ordering)
    }
    // build program queue
    progQueueHead = malloc(sizeof(PCB));
    copyContents(progQueueHead, head);
    PCB *currProg = progQueueHead;
    PCB *currReady = head;
    for(int i = 1; i < progNum; i++){
        PCB *newProg = malloc(sizeof(PCB));
        currReady = currReady->next;
        copyContents(newProg, currReady);
        currProg->next = newProg;
        currProg = newProg;
    }
    runQueue(progNum);
    freeProgQueue(progNum);
    freeFrameStr();
    framestr_init();
}

void copyContents(PCB *prog, PCB *ready)
{
    prog->pid = ready->pid;
    prog->startMem = ready->startMem;
    prog->len = ready->len;
    prog->pc = ready->pc;
    int ptSize = (int) prog->len/3 + 1;
    for(int i = 0; i < ptSize; i++) prog->pageTable[i] = ready->pageTable[i];
    prog->scriptName = ready->scriptName;
}

void freeProgQueue(int progNum)
{
    PCB *prev = progQueueHead;
    PCB *curr;
    for(int i = 0; i < progNum; i++){
        curr = prev->next;
        free(prev);
        prev = curr;
    }
}

void runQueue(int progNum)
{
    if (strcmp(policy, "FCFS") == 0 || strcmp(policy, "SJF") == 0)
    {
        FrameSlice *currFSlice;
        char *currCommand;
        int frameNum, position;
        for (int i = 0; i < progNum; i++)
        {
            // execute the entire program at the head
            for (int j = 0; j < head->len; j++)
            {
                frameNum = head->pageTable[(int) (head->pc - 1)/3];
                position =  frameNum*3 + (head->pc - 1) % 3;
                currFSlice = mem_get_from_framestr(position);
                currCommand = currFSlice->value;
                head->pc = (head->pc) + 1; // increment pc
                parseInput(currCommand);   // from shell, which calls interpreter()
            }

            // remove script course code from shellmemory and dequeue (clean up)
            int numOfPages = (int) (head->len + 2)/3 ; //round up
            int currFrameIndx;
            for (int k = 0; k < numOfPages; k++)
            {
                currFrameIndx = head->pageTable[k];
                for(int l = 0; l < 3; l++){
                    mem_remove_from_framestr(currFrameIndx*3 + l);
                }    
            }
            dequeue();
        }
    }
    else if (strcmp(policy, "AGING") == 0)
    {
        int timeSlice = 1;
        int startT = 0;
        int endT = 0;
        FrameSlice *currFSlice;
        char *currCommand;
        bool stopAging = false;
        int frameNum, position;

        while (head != NULL)
        {
            // execute one command
            frameNum = head->pageTable[(int) (head->pc - 1)/3];
            position =  frameNum*3 + (head->pc - 1) % 3;
            currFSlice = mem_get_from_framestr(position);
            currCommand = currFSlice->value;

            head->pc = (head->pc) + 1; // increment pc
            parseInput(currCommand);   // from shell, which calls interpreter()
            endT++;                    // increment time

            if (endT - startT >= timeSlice)
            {             // reassess after a time slice has passed
                endT = 0; // restart "timer"
                if (!stopAging)
                {
                    stopAging = age();
                    if (head->pc > head->len)
                    {
                        int numOfPages = (int) (head->len + 2)/3 ; //round up
                        int currFrameIndx;
                        for (int k = 0; k < numOfPages; k++)
                        {
                            currFrameIndx = head->pageTable[k];
                            for(int l = 0; l < 3; l++){
                                mem_remove_from_framestr(currFrameIndx*3 + l);
                            }    
                        }
                        dequeue();
                    }
                    // check if there is a prog with lenScore lower than that of the head
                    PCB *curr = head;
                    PCB *lowest = head;
                    PCB *lowestPrev = NULL;
                    PCB *lowestNext = lowest->next;
                    // find lowest score in queue
                    while (curr->next != NULL)
                    {
                        if (curr->next->lenScore < lowest->lenScore)
                        {
                            if (curr != head)
                                lowestPrev = curr;
                            lowest = curr->next;
                            lowestNext = lowest->next;
                        }
                        curr = curr->next;
                    }
                    // if prog with lowest score is something other than head, we promote it
                    // promote: put current head at the end of the queue and prog with new lowest score at the head
                    if (lowest != head)
                    {
                        // find tail
                        PCB *tail;
                        curr = head;
                        while (curr->next != NULL)
                            curr = curr->next;
                        tail = curr;
                        PCB *headNext = head->next;

                        // place head at the end, after tail
                        head->next = NULL;
                        tail->next = head;

                        // remove lowest from within the queue and place it at the head
                        if (lowestPrev != NULL)
                        { // lowest was not right after head
                            lowestPrev->next = lowestNext;
                            lowest->next = headNext;
                        } // else lowest is already at head position

                        head = lowest;
                    }
                }
                else
                { // aging stopped
                    if (head->pc > head->len)
                    {
                        int numOfPages = (int) (head->len + 2)/3 ; //round up
                        int currFrameIndx;
                        for (int k = 0; k < numOfPages; k++)
                        {
                            currFrameIndx = head->pageTable[k];
                            for(int l = 0; l < 3; l++){
                                mem_remove_from_framestr(currFrameIndx*3 + l);
                            }    
                        }
                        dequeue();
                    }
                }
            }
        }
    }
    else if (strcmp(policy, "RR") == 0)
    {
        int quantum = 2;
        FrameSlice *currFSlice;
        char *currCommand;
        PCB *currPCB, *nextPCB;
        currPCB = head;
        nextPCB = currPCB->next;
        int frameNum, position;
        bool pagefault = false;
        while (head != NULL)
        {
            for (int j = 0; j < quantum; j++)
            {
                frameNum = currPCB->pageTable[(int) (currPCB->pc - 1)/3];
                // check for fage fault
                if(frameNum == -1){
                    pagefault = true;
                    // missing page is brought into frame store
                    loadPage(currPCB);
                    break;
                }
                position =  frameNum*3 + (currPCB->pc - 1) % 3;
                currFSlice = mem_get_from_framestr(position);
                currCommand = currFSlice->value;
                parseInput(currCommand);         // from shell, which calls interpreter()
                currPCB->pc = (currPCB->pc) + 1; // increment pc
                if (currPCB->pc > currPCB->len)
                    break; // break if we've reached the end of the prog
            }

            if (currPCB->pc > currPCB->len)
            { // if we executed everything, remove from ready queue and go to next prog
                int pidToRemove = currPCB->pid;
                if (currPCB->next == NULL)
                    currPCB = head;
                else
                    currPCB = currPCB->next;
                removeFromQueue(pidToRemove);
            } // else, go to next prog
            else if(pagefault){
                PCB *next;
                if(currPCB->next == NULL){
                    next = head;
                }  
                else{
                    next = currPCB->next;
                    moveToBackOfQueue(currPCB->pid);
                }
                currPCB = next;
            }
            else{
                if (currPCB->next == NULL)
                    currPCB = head;
                else
                    currPCB = currPCB->next;
            }
        }
    }
}

int loadPage(PCB *pcb){
    // Open file
    char file[100] = "backingStore/";
    strcat(file, pcb->scriptName);
    FILE *p = fopen(file, "rt");
    if (p == NULL)
        return badcommandFileDoesNotExist();

    char line[1000];
    char emptyLine[] = "EMPTY";

    // skip the first lines up to PC
    for(int i = 0; i < pcb->pc - 1; i++){
        fgets(line, 999, p);
        memset(line, 0, sizeof(line));
    }

    int numPagesInFrameStr = pcb->pc / pageSize;

    // Insert up to page size lines into frame store
    int position;
    // Try to insert first line
    fgets(line, 999, p);
    position = insert_framestr(pcb->pid, pcb->pc/pageSize, line);

    if(position != 1001) // if frame store was not full
    {
        memset(line, 0, sizeof(line));
        for(int i = 1; i < pageSize; i++){
            if(!feof(p)){
                fgets(line, 999, p);
                position = insert_framestr(pcb->pid, pcb->pc/pageSize, line);
                memset(line, 0, sizeof(line));
            }
            else break;
        }

        // Update page table
        pcb->pageTable[numPagesInFrameStr] = position / pageSize;

        //Correction
        while ((position+1) % pageSize != 0)
        {
            insert_framestr(pcb->pid, pcb->pc/pageSize, emptyLine);
            position++;
        }
    }
    else // if from store was full, evict a page
    {
        int LRU_index = getLRUFrameNum()*3;
        FrameSlice *toEvict = mem_read_from_framestr(LRU_index);
        
        int evictId = toEvict->pid;
        int evictPn = toEvict->pageNum;

        // find evicted pcb
        PCB *evicted = progQueueHead;
        while(evicted->pid != evictId && evicted->next != NULL){
            evicted = evicted->next;
        }

        printf("Page fault! Victim page contents:\n");
        //printf("Victim program number: %d\n", evictId);
        //printf("Victim page number: %d\n", evictPn);
        //printf("Victim page contents:\n");
        for(int i = 0; i < pageSize; i++){
            printf("%s", mem_read_from_framestr(evicted->pageTable[evictPn]*3 + i)->value);
            mem_remove_from_framestr(evicted->pageTable[evictPn]*3 + i);
        }
        printf("End of victim page contents.\n");

        // Update page table of evicted pcb
        evicted->pageTable[evictPn] = -1; 


        // Try inserting again
        position = insert_framestr(pcb->pid, pcb->pc/pageSize, line);
        memset(line, 0, sizeof(line));
        for(int i = 1; i < pageSize; i++){
            if(!feof(p)){
                fgets(line, 999, p);
                position = insert_framestr(pcb->pid, pcb->pc/pageSize, line);
                memset(line, 0, sizeof(line));
            }
            else break;
        }

        // Update page table of pcb in question
        pcb->pageTable[numPagesInFrameStr] = position / pageSize;

        //Correction
        while ((position+1) % pageSize != 0)
        {
            insert_framestr(pcb->pid, pcb->pc/pageSize, emptyLine);
            position++;
        }
    }

    fclose(p);
}

// Decrease lenScore of all PCBs by 1, except for the head
// Length score cannot be lower than 0
// Return whether all scores are 0
bool age()
{
    PCB *curr = head;
    bool allZeros = head->lenScore == 0;
    while (curr->next != NULL)
    {
        curr = curr->next;
        if (curr->lenScore > 0)
        {
            allZeros = false;
            curr->lenScore = curr->lenScore - 1;
        }
    }
    return allZeros;
}

void moveToBackOfQueue(int pid){
    // PCB is not the head
    if(pid != head->pid){
       // get previous, curr, and post PCBs
       PCB *prevPCB = head;
       PCB *currPCB = prevPCB->next;
       while(currPCB->pid != pid){
           prevPCB = currPCB;
           currPCB = currPCB->next;
       }
       PCB *postPCB = currPCB-> next;

       // remove curr PCB from middle of queue
       prevPCB->next = postPCB;

       // get last PCB
       PCB *end = head;
       while(end->next != NULL) end = end->next;
       end->next = currPCB;
       currPCB->next = NULL;
    } 
    // PCB is the head and not the only one
    else if(pid == head->pid && head->next != NULL){
       PCB *currPCB = head;
       PCB *second = head->next;

       // update head
       head = second;

       // get last PCB
       PCB *end = head;
       while(end->next != NULL) end = end->next;
       end->next = currPCB;
       currPCB->next = NULL;
    }
    // If PCB is the head and the only one, do nothing
}

// Add new PCB at the end of the queue
// Return its pid
int enqueue(int start, int len, int pageTable[], char *name)
{
    if (head == NULL)
    {
        head = malloc(sizeof(PCB));
        head->pid = ++latestPid;
        head->startMem = start;
        head->len = len;
        head->lenScore = len;
        head->pc = 1;
        int ptSize = (int) len/3 + 1;
        for(int i = 0; i < ptSize; i++) head->pageTable[i] = pageTable[i];
        head->scriptName = name;
        head->next = NULL;
        return head->pid;
    }
    else
    {
        PCB *current = head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        PCB *new = malloc(sizeof(PCB));
        current->next = new;
        new->pid = ++latestPid; // unique pid
        new->startMem = start;
        new->len = len;
        new->lenScore = len;
        new->pc = 1;
        int ptSize = (int) len/3 + 1;
        for(int i = 0; i < ptSize; i++) new->pageTable[i] = pageTable[i];
        new->scriptName = name;
        new->next = NULL;
        return new->pid;
    }
}

// Add new PCB at the head of the queue
// Return its pid
int prepend(int start, int len, int pageTable[], char *name)
{
    if (head == NULL)
    {
        head = malloc(sizeof(PCB));
        head->pid = ++latestPid;
        head->startMem = start;
        head->len = len;
        head->lenScore = len;
        head->pc = 1;
        int ptSize = (int) len/3 + 1;
        for(int i = 0; i < ptSize; i++) head->pageTable[i] = pageTable[i];
        head->scriptName = name;
        head->next = NULL;
    }
    else
    {
        PCB *new = malloc(sizeof(PCB));
        new->pid = ++latestPid; // unique pid
        new->startMem = start;
        new->len = len;
        new->lenScore = len;
        new->pc = 1;
        int ptSize = (int) len/3 + 1;
        for(int i = 0; i < ptSize; i++) new->pageTable[i] = pageTable[i];
        new->scriptName = name;
        new->next = head;
        head = new;
    }
    return head->pid;
}

// Add a PCB to an ordered queue (increasing by length)
// Return pid
int insertInQueue(int start, int len, int pageTable[], char *name)
{
    if (head == NULL)
        return enqueue(start, len, pageTable, name);
    else if (len < head->len)
        return prepend(start, len, pageTable, name);
    else
    {
        PCB *curr = head;
        while (curr->next != NULL)
        {
            if (len < curr->next->len)
            {
                PCB *new = malloc(sizeof(PCB));
                new->pid = ++latestPid;
                new->startMem = start;
                new->len = len;
                new->lenScore = len;
                new->pc = 1;
                int ptSize = (int) len/3 + 1;
                for(int i = 0; i < ptSize; i++) new->pageTable[i] = pageTable[i];
                new->scriptName = name;
                PCB *next = curr->next;
                curr->next = new;
                new->next = next;
                return new->pid;
            }
            curr = curr->next;
        }
        return enqueue(start, len, pageTable, name); // if program wasn't placed in the queue, add it to the end
    }
}

// Remove PCB at the head of the queue
// Return its pid
int dequeue()
{
    PCB **head_ptr = &head;

    //  Checks if queue is empty
    if (*head_ptr == NULL) return -1;
    
    int retpid = (*head_ptr)->pid;
    PCB *next_pcb = (*head_ptr)->next;
    free(*head_ptr);
    head = next_pcb;

    return retpid;
}

// Remove PCB with given pid from the queue (and free memory space)
void removeFromQueue(int pid)
{
    bool cont;
    PCB *currPCB;
    if (pid == head->pid)
    {
        dequeue();
        cont = false;
    }
    else
    {
        currPCB = head;
        cont = true;
    }

    while (cont)
    {
        if (currPCB->next == NULL)
            cont = false;
        else if (currPCB->next->pid == pid)
        {
            PCB *toRemove = currPCB->next;
            if (currPCB->next->next == NULL)
                currPCB->next = NULL;
            else
                currPCB->next = currPCB->next->next;
            free(toRemove);
            cont = false;
        }
        else
            currPCB = currPCB->next;
    }
}