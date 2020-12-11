#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <queue>
#include <semaphore.h>
#include <cstdlib>
#include <ctime>

using namespace std;

#define NUM_THREADS 5
#define MEMORY_SIZE 1000

struct node
{
    int id;
    int size;
};


queue<node> myqueue; // shared que
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores

int thread_message[NUM_THREADS]; // thread memory information
char  memory[MEMORY_SIZE]; // memory size
int sharedCount=0; //created a global sharedCount to keep track of how many threads have requested for memory
int lastIndex=-1; //lastIndex keep the lastIndex that have been aloocated in the memory array

void release_function() //it's called at the end of program, killing threads are done at the end of functions
{
    exit(0);
    //This function will be called
    //whenever the memory is no longer needed.
    //It will kill all the threads and deallocate all the data structures.
}

void my_malloc(int * thread_id, int size) //to allocate memory, write it to the queue with thread id and size of the memory that is requested
{
    pthread_mutex_lock(&sharedLock); //lock the mutex since we're accessing a global variable
    int * idOfThread = (int *) thread_id;
    node Node;
    Node.id= * idOfThread;
    Node.size= size;
    myqueue.push(Node);
    pthread_mutex_unlock(&sharedLock); //unlock the mutex
    //This function will add the struct to the queue
}

void * server_function(void *)
{
    //This function should grant or decline a thread depending on memory size.
    while(sharedCount != NUM_THREADS){ //server function will keep checking the queue until all the threads have request a memory
        while(myqueue.empty()); //while it's empty just wait in the while loop
        pthread_mutex_lock(&sharedLock); //lock the mutex because we're accessing global variables
        node N=myqueue.front(); //take the front node of the myqueue
        int idThread=N.id; //take the id of this node
        int sizeRequested=N.size; //take the size requested by the thread
        
        if(MEMORY_SIZE-(lastIndex+1) >= sizeRequested){ //if there are enough memory for this request
            thread_message[idThread]=lastIndex+1; //update the thread_message array's index idThread with the corresponding lastIndex
            lastIndex += sizeRequested; //update the lastIndex
        }
        else{
            thread_message[idThread]= -1; //if there are no enough memory then equal the thread_message's corresponding index of idThread to -1
        }
        myqueue.pop(); //pop the node
        sem_post(&semlist[idThread]); //unlock the semaphore of the thread so that it can fill the memory with 1's
        pthread_mutex_unlock(&sharedLock); //unlock the mutex
    }
    
    pthread_exit(NULL); //exit the thread when the sharedCount is equal to NUM_THREADS
}

void * thread_function(void * id) //thread function
{
    //This function will create a random size, and call my_malloc
    //Block
    //Then fill the memory with 1's or give an error prompt
    int *idOfTheThread= (int *) id;
    int randomSize=rand()%(MEMORY_SIZE/3); //create a randomSize
    
    my_malloc((int *)id, randomSize); //call the my_malloc function to create the node to add it to the queue
    sem_wait(&semlist[* idOfTheThread]); //unlock the semaphore of the thread
    
    pthread_mutex_lock(&sharedLock); //lock the mutex since we're accessing global variables
    int beginning=thread_message[*idOfTheThread]; //beginning of the index for thread to fill up the memory array
    
    if(beginning >= 0){ //if beginnig is greater than 0 than we can say that server thread found a place for the memory
        for(int i=0; i< randomSize; i++){ //fill the memory array
            memory[beginning]= '1';
            beginning++;
        }
    }
    else{ //if beginning is -1 then server cannot find appropriate place fot the thread, print message
        cout << "Thread "<< *idOfTheThread << ": Not enough memory"<<endl;
    }
    
    sharedCount++; //increment sharedCount since the thread requested a memory
    pthread_mutex_unlock(&sharedLock); //unlock the mutex
    pthread_exit(NULL); //exit the thread
}

void init()
{
    pthread_mutex_lock(&sharedLock);    //lock
    for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
    {sem_init(&semlist[i],0,0);}
    for (int i = 0; i < MEMORY_SIZE; i++)    //initialize memory
      {char zero = '0'; memory[i] = zero;}
    pthread_create(&server,NULL,server_function,NULL); //start server
    pthread_mutex_unlock(&sharedLock); //unlock
}

void dump_memory()
{
 // You need to print the whole memory array here.
    for(int i=0; i< MEMORY_SIZE; i++){ //print the memory
        cout << memory[i] << " ";
    }
}

int main (int argc, char *argv[])
{
    srand(time(0));
    int threadID[NUM_THREADS]; //int array to keep the id's of threads
    
    for (int j=0; j< NUM_THREADS; j++){ //assign the id's for the threads
        threadID[j]=j;
    }

     //You need to create a thread ID array here

    init();    // call init
    
    pthread_t threads[NUM_THREADS]; //pthread array to keep the pthreads
    for (int i=0; i< NUM_THREADS; i++) //create the threads
    {
        pthread_create(&threads[i], NULL, thread_function, (void *) & threadID[i]);
    }
     //You need to create threads with using thread ID array, using pthread_create()
     
    for (int i=0; i< NUM_THREADS; i++){ //join them
        pthread_join(threads[i], NULL);
    }

     //You need to join the threads

     dump_memory(); // this will print out the memory
     printf("\nMemory Indexes:\n" );
     for (int i = 0; i < NUM_THREADS; i++)
     {
         printf("[%d]" ,thread_message[i]); // this will print out the memory indexes
     }
     printf("\nTerminating...\n");
     release_function(); //finish the program
    
    return 0;
}

