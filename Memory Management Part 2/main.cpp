/*Deniz Cangi-25427-CS307 HW4*/
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

struct node{ //linkedlist node to use in linkedlist, it consists of id, size, index and next node
    int id;
    int size;
    int index;
    node * next;
    
    node(int i=-1, int s=0, int ix=0, node * n=NULL): id(i), size(s), index(ix), next(n) {}
};

class linkedlist{ //linkedlist class
    
private:
    node * head;
    
public:
    linkedlist(){head=NULL;} //constructor
    ~linkedlist(){ //destructor
        node * ptr=head;
        while(ptr!=NULL){
            ptr=ptr->next;
            delete head;
            head=ptr;
        }
    }
    node * returnhead(){ //return the head
        return head;
    }
    void addinorder(node * newnode){ //add element to list
        node * current;
        if(head==NULL){
            newnode->next=head;
            head=newnode;
        }
        else{
            current=head;
            while(current->next!=NULL && current->next->index < newnode->index){
                current=current->next;
            }
            newnode->next=current->next;
            current->next=newnode;
        }
    }
    
    void deleteLinkedList(){ //delete and deallocate the memory
        node * temp=head;
        while(temp!=NULL){
            temp=temp->next;
            delete head;
            head=temp;
        }
    }
};

#define NUM_THREADS 3
#define MEMORY_SIZE 10

struct nodeQueue //nodeQueue is for queue
{
    int id;
    int size;
};


queue<nodeQueue> myqueue; // shared que
pthread_mutex_t sharedLock= PTHREAD_MUTEX_INITIALIZER; //mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores
linkedlist mem; //linked list to keep the memory

char  memory[MEMORY_SIZE]; // memory array
bool finish=false; //bool to finish the execution of the program after 10 seconds

void dump_memory();

void release_function() //it's called at the end of program, deallocate the memory and memory returned to its initial state
{
    mem.deleteLinkedList(); //call the delete function to deallocate
    for(int i=0; i<MEMORY_SIZE; i++){ //return its initial state
        memory[i]='X';
    }
    exit(0); //exit the program
}

void use_mem() //to use memory for the thread for a random amount of time between 1 to 5 seconds
{
    int randSleep=(rand()%5)+1;
    sleep(randSleep);
}

void free_mem(int index){ //this function is to free the memory that the thread used after it finish using it, it gets the index of the node where the memory is used
    
    node * ptr=mem.returnhead();
    node * prev=NULL;
    while(ptr->index!=index){ // find the node
        prev=ptr;
        ptr=ptr->next;
    }

    int ix=index; //index of the node
    int size = ptr->size; //size of the node
    node * next = ptr->next; //next keep track of next node of ptr
     
    for (int i = 0; i < size; i++) //change the memory array with X
    {
        memory[ix++] = 'X';
    }
    
    if (next != NULL  && prev != NULL) //if both of previous and next node exist
    {
        if (next->id == -1 && prev->id == -1) //if both of them are free
        {
            int newsize= prev->size + ptr->size + next->size; //new size of the prev node
            prev->size = newsize; //change the size of it
            prev->next = next->next; //next of the prev is next's next
            delete ptr; //dleete ptr and next
            delete next;

        }
        else if (next->id == -1 && prev->id != -1) //if only the next is free
        {
            ptr->id=-1; //change the if of the ptr
            ptr->size = ptr->size+next->size; //change the size of it
            ptr->next = next->next; //next is next's next
            delete next; //delete next node
        }
        
        else if (next->id != -1 && prev->id == -1) //if prev is free
        {
            prev->size = prev->size +ptr->size; //change the size of th prev node
            prev->next = next; //next of prev become next
            delete ptr; //dleete ptr
        }
        
        else //if both of them are full then just change the id of the ptr
        {
            ptr->id = -1;
        }
    }
    
    else if (next != NULL && prev == NULL) //if the ptr is the first node- head of the linked list
    {
        if (next->id != -1) //if next's id is not -1 then just change the id of the ptr
            ptr->id = -1;
        else //if next is free too, then merge them in the ptr and delete the next node
        {
            ptr->id=-1;
            ptr->size =ptr->size+next->size;
            ptr->next = next->next;
            delete next;
        }
    }
    
    else if (next== NULL && prev != NULL) //if the ptr is the last element of the linked list
    {
        if (prev->id != -1) //if prev is full then change the id of the ptr
            ptr->id = -1;
        else //if prev is free too then merge them in the prev and delete the ptr
        {
            prev->size =prev->size+ptr->size;
            prev->next = ptr->next;
            delete ptr;
        }
    }
}

bool my_malloc(int * thread_id, int size, int & index) //my_malloc function put the requested size and id of the thread to the queue and find the index if the allocation is completed, it changes the index variable and return true if allocation is successful
{
    int * idOfThread = (int *) thread_id;
    nodeQueue Node;
    Node.id= * idOfThread;
    Node.size= size;
    myqueue.push(Node);
    pthread_mutex_unlock(&sharedLock); //unlock the mutex
    sem_wait(&semlist[* idOfThread]);//lock the semaphore of the thread
    pthread_mutex_lock(&sharedLock); //lock it again since we're looking in linkedlist

    node *ptr=mem.returnhead();
    while(ptr!=NULL){ //find the node in the linked list if it's allocated, if not it returns false
        if(ptr->id== * idOfThread){
            index=ptr->index;
            return true;
        }
        ptr=ptr->next;
    }
    return false;
}

void * server_function(void *)
{
    while(!finish){ //it continues until the finish is true
        pthread_mutex_lock(&sharedLock); //lock the mutex, we're checking for queue
        if(myqueue.empty()==false){ //if it's not empty
            
            nodeQueue N=myqueue.front(); //get the node, id and requested size of the thread
            int idThread= N.id;
            int sizeRequested=N.size;
            
            node * ptr=new node;
            node * head= mem.returnhead();
            node * temp = head;
            bool check=false;
            
            while(temp!=NULL && check==false) //find an approporiate place with first fit algorithm, if not found then check remains false
            {
                if(temp->id==-1 && temp->size >= sizeRequested) // we look for the id's of nodes and the size of the node if it's enpugh for the requested size of the thread
                {
                    ptr=temp;
                    check=true;
                }
                else{
                    temp=temp->next;
                }
            }
            
            if(check) //if we find a place for the thread in the memory
            {
                if(ptr==head){ //if we find the node in head
                    if(ptr->size==sizeRequested){ //if size's are same
                        ptr->id=idThread; //change the id
                    }
                    else{ //if not, create a new node which keep the remaining free space and insert it to the linked list
                        ptr->next = new node(-1,((ptr->size) - sizeRequested),((ptr->index) + sizeRequested),ptr->next);
                        ptr->size = sizeRequested;
                        ptr->id = idThread;
                    }
                }
                
                else{ //if it's not head
                    if(ptr->size==sizeRequested){ //if size's are same
                        ptr->id=idThread; //change the id
                    }
                    else{
                        
                        node * nextempty=new node(-1, ((ptr->size)-sizeRequested),((ptr->index)+sizeRequested),ptr->next); //create a new node which keeps the remaining memory
                        ptr->next = nextempty; //next becomes this free node
                        ptr->id = idThread; //change the id
                        ptr->size = sizeRequested; //change the size
                    }
                }

                int ix=ptr->index; //ix is the index of the ptr
                char ch=(char)ptr->id+48;
            
                for (int i=0; i< sizeRequested; i++) //fill the memory array with the thread's id
                {
                    memory[ix++]=ch;
                }
            
                myqueue.pop(); //pop the element from the queue
                sem_post(&semlist[idThread]); //open the semaphore
                dump_memory();
            }
        
            else{
                myqueue.pop(); //if it cannot find a memory then just pop the element and open up the semaphore
                sem_post(&semlist[idThread]);
            }
            pthread_mutex_unlock(&sharedLock); //unlock and lock the mutex again since it's going to check for queue again
            pthread_mutex_lock(&sharedLock);
        }
        pthread_mutex_unlock(&sharedLock); //unlock it again if the queue is empty
    }
    pthread_exit(NULL); //when the program finished, exit the thread
    
}

void * thread_function(void * id) //thread function
{
    while(!finish){ //while finish is false
        
        int randomSize=rand()%(MEMORY_SIZE/3)+1; //create a randomSize
        
        pthread_mutex_lock(&sharedLock); //lock the mutex since we're accessing a global variable
        int index;
        bool check= my_malloc((int *)id, randomSize, index); //call my malloc to allocate the memory
    
        pthread_mutex_unlock(&sharedLock);
        if (check){ //if we can allocate the memory
            use_mem(); //use the memory
            pthread_mutex_lock(&sharedLock);
            free_mem(index); //free the memory
            pthread_mutex_unlock(&sharedLock);
        }
    }
    pthread_exit(NULL); //when program finishes exit the thread
}

void init()
{
    pthread_mutex_lock(&sharedLock);    //lock
    for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
    {sem_init(&semlist[i],0,0);}
    pthread_create(&server,NULL,server_function,NULL); //start server
    pthread_mutex_unlock(&sharedLock); //unlock
}

void dump_memory()
{
    node * ptr=mem.returnhead();
    while(ptr!=NULL){ //print the linked list
        cout << "["<<ptr->id<<"]["<<ptr->size<<"]["<<ptr->index<<"]";
        ptr=ptr->next;
        if(ptr!=NULL)
            cout << "---";
    }
    cout << endl << "Memory Dump:"<<endl;
    
    for(int i=0; i< MEMORY_SIZE; i++){ //print the memory array
        cout << memory[i] << " ";
    }
    cout << endl << "*************************************" << endl;
}

int main (int argc, char *argv[])
{
    srand(time(0));
    node * FirstNode= new node(-1, MEMORY_SIZE,0,NULL);
    mem.addinorder(FirstNode); //add the first node to the list where it's a free node with size MEMORY_SIZE
    
    for (int i=0; i<MEMORY_SIZE; i++){ //initialize the memory array
        memory[i]='X';
    }
    
    int threadID[NUM_THREADS]; //int array to keep the id's of threads
    
    for (int j=0; j< NUM_THREADS; j++){ //assign the id's for the threads
        threadID[j]=j;
    }

    init();    // call init
    
    pthread_t threads[NUM_THREADS]; //pthread array to keep the pthreads
    for (int i=0; i< NUM_THREADS; i++) //create the threads
    {
        pthread_create(&threads[i], NULL, thread_function, (void *) & threadID[i]);
    }
    
    int sec=10; //sleep for 10 seconds then equal finish to true to exit the threads
    sleep(sec);
    finish=true;
    
    release_function(); //finish the program with deallocation and return the array to it's initial state
    
    return 0;
}
