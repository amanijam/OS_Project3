# ECSE 427/COMP 310
# Assignment 3: Memory Management
## Authors:
### Names & McGill IDs
* Amani Jammoul 260381641

* Annabelle Dion 260928845

## In this folder:
* Makefile

* shell.c, shell.h

* interpreter.c, interpreter.h

* shellmemory.c, shellmemory.h

* scheduler.c, scheduler.h

### (given test cases)
* prog1-prog12

* tc1, tc2, tc3, tc4, tc5 (with results)

## Notes:
Our code builds off of our A2 submission code. Everything added onto our previous submission is entirely our work.

## Assumptions:
* framesize and varmemsize are no more than 1000. 

* framesize >= 18 for exec commands (enough space to load at least 2 pages of 3 programs)

## Brief Description:
### Assignment 1
For assignment 1 we used the given started code to begin with, so that logic remains. 

### Assignment 2
For assignment 2, we added a scheduler.c file which is used for scheduling and multi-process execution. This file and its methods are called when the "exec" command is inputted. It is able to handle FCFS, SJF, RR, and AGING scheduling policies. As in the assignment description, SJF executes the programs in order from least number of lines to the most. RR rotates through the progams, executing 2 lines at a time (which are originally ordered as FCFS). AGING uses the SJF ordering and an aging mechanism (with timestamp = 1 line) to promote jobs when necessary. A large part of our scheduler relies on properly maintaining a PCB queue. The queue can be assesed from the global *head* PCB and each PCB has a *next* attribute which points to the following PCB in the queue.

### Assignment 3
For assignment 3, we built on our scheduler and changed our shell memory structure. Instead of having one array as our memory which holds both variables and instructions, we split it into two: a variable store and a frame store. Our variable store has the same structure as the previous shellmemory, only now it contains variables alone. Our frame store is an array of FrameSlices, which is a struct that holds a program pid, the program's page number, and a value (which is an instruction). The sizes of these stores are dynamically set to the inputs "framesize" and "varmemsize" and maintained within the initialization and insert functions. Furthermore, each PCB has (in addition to a pid, length (num of lines), lenScore (used in the AGING policy), and PC) a page table array that stores the frame number at which its pages lie in the frame store.

At the start of our scheduler execution, the first 2 pages of each program is loaded into the frame store (as opposed to the entire script, as it was previously). Our ready queue remains the same as in assignemnt 2, however we now also have a progam queue, which is just a queue with all the PCBs. This was necessary because in our implementation, whenever a program is done executing, it is removed from the ready queue immediately, making it impossible to access its PCB information afterwards.

Situations where program pages that need to be accessed are not yet in the frame store are handled using page faults, which essentially attempts to load the missing page into the frame store. If the frame store is empty, a victim page is evicted (here, it is the LRU frame) and the necessary page is loaded into the frame store.

In order to handle this, our shellmemory now maintains an LRU_Queue, with FrameNodes, and an LRU_Hash. This is used to keep track of the least recently used frame in the frame store. When the LRU frame is requested (from the scheduler), a function is called to return the LRU frame number (the FrameNode at the tail of the queue). The LRU_Queue and LRU_Hash are maintained and updated whenever a frame is "refered to", i.e. is inserted into the frame store or retrieved. A function must also be called from the scheduler whenever a frame is evicted, in order to update these structures.

## Assignment Instructions

![A3-1 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/2d08eb07-1537-47aa-b64c-96b854f80ace)
![A3-2 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/6335173f-f2b9-4858-861c-43395df79122)
![A3-3 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/b804adc9-53c3-487c-a434-c8568ed932c2)
![A3-4 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/427b80c2-be53-4e91-9b9d-00069204af81)
![A3-5 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/226102af-e34d-4068-9f60-bbc2493e1787)
![A3-6 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/c066d7f2-f497-4d86-9855-6e0b25eb175b)
![A3-7 of 7](https://github.com/amanijam/OS_Project3/assets/19826681/a1becf73-3573-470f-82c1-62caa0a695dc)
