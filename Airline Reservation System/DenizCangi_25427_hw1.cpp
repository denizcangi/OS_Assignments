/* Deniz Cangi 25427 HW1 CS307*/
#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <ctime>

using namespace std;

int turn=0;//initially turn is 0, which indicates TravelAgency1 will start first
int M[2][50]; //plane matrix with 100 seats
bool empty=true; //boolean variable which checks if there's any empty seat in the plane
int cnt=0; //integer variable to keep track of how many seats are reserved by the agencies

void *agency1(void *) //TravelAgency1 thread's function
{
    while(empty){ //while there is empty seat in plane
        int i;
        int j;
        bool found=false;
        
        while(turn!=0){ //for strict alternation, while turn is not equal to 0, TravelAgency1 waits
            if(!empty){ //while it's waiting for other threads if the plane gets full then it exits the thread.
                pthread_exit(NULL);
            }
        }
    
        while(!found){ //while an empty seat is not found
            i=rand()%2; //randomly generate a row index for seat
            j=rand()%50; //randomly generate a column index for seat
        
            if(M[i][j]==0){ //if the randomly selected seat is empty
                found=true; //get out of the loop
            } //if not empty then go back to the beginning of while function and randomly generate a new seat on the plane
        }
    
        M[i][j]=1; //set this seat as 1 to indicate that it's reserved by first agency

        cnt++; //,increment the count since it reserved a seat
        
        if(i==0) //to cout the reserved seat number on screen
        {
            cout << "Seat number "<< j*2+1 << " is reserved by TravelAgency1."<<endl;
        }    
        else if(i==1)
        {
            cout << "Seat number "<< (i+j)*2 << " is reserved by TravelAgency1."<<endl;
        }
        turn=1; //make turn equal to 1 to give its turn to second agency
        
    }
    pthread_exit(NULL); //to exit the thread
}

void *agency2(void *){
    
    while(empty){

        int i;
        int j;
        bool found=false;

        while(turn!=1){
            
            if(!empty)
            {
                pthread_exit(NULL);
            }
        }
    
        while(!found){
            i=rand()%2;
            j=rand()%50;
        
            if(M[i][j]==0){
                found=true;
            }
        }
        
        M[i][j]=2;
        cnt++;
    
        if(i==0)
        {
            cout << "Seat number "<< j*2+1 << " is reserved by TravelAgency2."<<endl;
        }
    
        else if(i==1)
        {
            cout << "Seat number "<< (i+j)*2 << " is reserved by TravelAgency2."<<endl;
        }
    
        turn=2;
    }
    pthread_exit(NULL);
}

void *agency3(void *){
    
    while(empty){
       
        int i = 0;
        int j;
        bool found=false;

        while(turn!=2){
            
            if(!empty)
                pthread_exit(NULL);
        }
    
        while(!found){
            i=rand()%2;
            j=rand()%50;
        
            if(M[i][j]==0){
                found=true;
            }
        }
    
        M[i][j]=3;
        cnt++;
    
        if(i==0)
        {
            cout << "Seat number "<< j*2+1 << " is reserved by TravelAgency3."<<endl;
        }
    
        else if(i==1){
            cout << "Seat number "<< (i+j)*2 << " is reserved by TravelAgency3."<<endl;
        }
        turn=0;

     }
    pthread_exit(NULL);
}

int main(){
    
    srand(time(0)); //to randomly generate number for seats
    pthread_t TravelAgency1;
    pthread_t TravelAgency2;
    pthread_t TravelAgency3;
    
    pthread_create(&TravelAgency1,NULL,agency1,NULL); //create three distinct threads for each agency
    pthread_create(&TravelAgency2,NULL,agency2,NULL);
    pthread_create(&TravelAgency3,NULL,agency3,NULL);
    
    do{ //check for count to change the variable empty
        
        if(cnt > 99) //if cnt>99 i.e. cnt=100 empty become false, because the plane is full.
            empty=false;
        
    }while(empty);
        
    pthread_join(TravelAgency1, NULL); //join the threads
    pthread_join(TravelAgency2, NULL);
    pthread_join(TravelAgency3, NULL);
    
    
    for(int i=0;i<2;i++){ //print the matrix which shows which agency reserved which seat
        for(int j=0;j<50;j++){
            cout << M[i][j]<<" ";
        }
        cout <<endl;
    }
    return 0;
}
