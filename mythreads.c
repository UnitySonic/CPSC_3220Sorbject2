#include "mythreads.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <stdbool.h>
#include <assert.h>


/**
 * Global variables!
*/
int interruptsAreDisabled = 0;
struct Node{
    struct Node* next;
    ucontext_t* data;
    int ID;
    void * result;
};

struct Lock
{
    int lockVal;
    int conditions[CONDITIONS_PER_LOCK];
};


struct Node * head = NULL;


int nextThreadID = 0;
bool runNewThread = false;
int currentThread = 0;

struct Lock locks[NUM_LOCKS];




//When a call to thread yield is made, threadIDtoUPDATE and threadContextToUpdate will have their info stored
//It will be the reponsibility of the function resuming to update these.
//int threadIDToUpdate;
//ucontext_t* threadContextToUpdate;

bool runNewThread;

void realThreadYield();
void threadMain(thFuncPtr threadfunc, void * argPtr);

static void interruptDisable()
{
    assert(!interruptsAreDisabled);
    interruptsAreDisabled = 1;
}

static void interruptEnable()
{
    assert(interruptsAreDisabled);
    interruptsAreDisabled = 0;
}





void addNode(ucontext_t*, int);

//May want to change this to delete based off nextThreadID
void delNode(ucontext_t*);



struct Node* findNode(int targetID)
{
    struct Node * traveler = head;

    while(traveler != NULL)
    {
        if(traveler->ID == targetID)
            return traveler;
        else
            traveler = traveler->next;
    }
    return NULL;
}

void addNode(ucontext_t* newData, int ID)
{
    //If head is null initialize head
    if(head == NULL)
    {
        head = (struct Node*) malloc(sizeof(struct Node));
        head->data = newData;
        head->ID = ID;
        head->next = NULL;
        head->result = NULL;
    }
    else
    {
        if(findNode(ID) == NULL)
        {
            //Otherwise travel to end of linkedlist and add a new node
            struct Node * traveler = head;
            while (traveler->next != NULL)
            {
                traveler = traveler->next;
            }
            traveler->next = (struct Node*) malloc(sizeof(struct Node));
            traveler->next->data = newData;
            traveler->next->ID = ID;
            traveler->next->next = NULL;
            traveler->next->result= NULL;
            }
    }
}


/**
 * Deletes a node from our global linked list. The void pointer provided must match a pointer
 * in the linked list for the removal to occur
*/
void delNode(ucontext_t* targetData)
{
    struct Node * traveler = head;
    struct Node * previousNode = head;

    //If the head has our target data, either set head to NULL or make the new
    //head the next node
    if(head->data == targetData)
    {
        if(head->next == NULL)
        {
            free(head);
            head = NULL;
        }
        else
        {
            head = head->next;
        }
        return;
    }

    else{
        //Increment till targetData found, delete it's node and connect linked list back together
           while(traveler != NULL)
           {
                if (traveler->data == targetData)
                {
                    previousNode->next = traveler->next;
                    free(traveler);
                    return;
                }
                else
                {
                    previousNode = traveler;
                    traveler = traveler->next;
                }
           }
        }
    }












void threadInit()
{
    ucontext_t mainContext;
    getcontext(&mainContext);
    addNode(&mainContext,nextThreadID++);

    for(int i = 0; i < NUM_LOCKS; i++)
    {
        locks[i].lockVal = 1;
        for (int j = 0; j < CONDITIONS_PER_LOCK; j++)
        {
            locks[i].conditions[j] = 0;
        }
    }

}


int threadCreate(thFuncPtr funcPtr, void *argPtr)
{
    interruptDisable();
    ucontext_t* newcontext = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext (newcontext ) ; // save the current context


    // allocate and initialize a new call stack
    newcontext->uc_stack.ss_sp = malloc (STACK_SIZE);
    newcontext->uc_stack.ss_size = STACK_SIZE;
    newcontext->uc_stack.ss_flags = 0;
    // modify the context , so that when activated
    // func will be called with the 3 arguments , x , y , and z
    // in your code , func will be replaced by whatever function you want
    // to run in your new thread .


    void (*mainthread) (thFuncPtr,void*) = threadMain;
    makecontext(newcontext , (void(*)(void))mainthread , 2, funcPtr, argPtr);

    int currentThreadID = nextThreadID++;

    addNode(newcontext,currentThreadID);
    runNewThread = true;

    interruptEnable();


    realThreadYield();

    return currentThreadID;

}

void threadYield()
{
    if(interruptsAreDisabled == 0)
    {
        realThreadYield();
    }
}
void realThreadYield()
{
    //Later I might make it so that the user calls threadYieldFake which calls threadYieldReal. Might make it easier to handle interrupts.
    struct  Node * newContext;

    int threadToSave;

    interruptDisable();

    if(runNewThread)
    {

        threadToSave = currentThread;
        runNewThread = false;
        newContext = findNode(nextThreadID-1);

    }
    else
    {


       struct Node * currentNode = findNode(currentThread);
       bool weGood = false;

       while (currentNode->next != NULL)
       {
            currentNode = currentNode->next;
            if(currentNode->result == NULL)
            {
                weGood = true;
                break;
            }

       }
       if(weGood == false)
       {

            newContext = head;
            threadToSave= currentThread;
            currentThread = 0;
       }
       else
       {

            newContext = currentNode;
            threadToSave= currentThread;
            currentThread = currentNode->ID;
       }

    }


    swapcontext(findNode(threadToSave)->data ,newContext->data);
    interruptEnable();

}
void threadJoin(int thread_id, void **result)
{
    interruptDisable();
    void * currentNodeResult = findNode(thread_id)->result;

    if(findNode(thread_id) == NULL)
    {
        interruptEnable();
        return;
    }
    else
    {


        if(currentNodeResult != NULL )
        {
            struct Node * targetThread = findNode(thread_id);


            free(targetThread->data);
            /*
            free(targetThread->result);
            free(targetThread);
            */
            *result = currentNodeResult;
            interruptEnable();
            return;

        }

        else
        {
            interruptEnable();
            for(;;)
            {
                interruptDisable();

                currentNodeResult = findNode(thread_id)->result;

                if(currentNodeResult != NULL)
                {
                    struct Node * targetThread = findNode(thread_id);


                    free(targetThread->data);
                    /*
                    free(targetThread->result);
                    free(targetThread);
                    */
                    *result = currentNodeResult;
                    interruptEnable();
                    return;
                }
                interruptEnable();
                realThreadYield();
            }
        }
    }

   return;
}

//exits the current thread -- calling this in the main thread should terminate the program
void threadExit(void *result)
{
    interruptDisable();

    if(currentThread == 0)
    {
        exit(0);
    }
    else
    {
        findNode(currentThread)->result = result;
    }

    interruptEnable();

    realThreadYield();
}

void threadMain(thFuncPtr threadfunc, void * argPtr)
{

    currentThread = nextThreadID-1;
    interruptEnable();
    void * result = threadfunc(argPtr);
    threadExit(result);
}




/**
 *
 * Synchornization SECTION!!!
*/

void threadLock(int lockNum)
{
    while(locks[lockNum].lockVal == 0)
    {
        realThreadYield();
    }

    locks[lockNum].lockVal = 0;


}
void threadUnlock(int lockNum)
{
    locks[lockNum].lockVal = 1;
}
void threadWait(int lockNum, int conditionNum)
{
    if(locks[lockNum].lockVal == 1)
    {
        fprintf(stderr, "This function was called incorrectly. The Mutex should  be LOCKED\n");
        exit(0);
    }
    else
    {
        threadUnlock(lockNum);
        while(locks[lockNum].conditions[conditionNum] == 0)
        {
            realThreadYield();
        }
        threadLock(lockNum);
        locks[lockNum].conditions[conditionNum] = 0;
    }

}
void threadSignal(int lockNum, int conditionNum)
{
    locks[lockNum].conditions[conditionNum] = 1;
}

//this needs to be defined in your library. Don't forget it or some of my tests won't compile.



