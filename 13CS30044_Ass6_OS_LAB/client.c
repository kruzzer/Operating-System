#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/shm.h>

#define mutex 0
#define MAX_BUFFER_SIZE 1024


void up(int semid , int subsemid)
{
    struct sembuf sop;
    sop.sem_num = subsemid;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if(semop(semid , &sop , 1) < 0 )
        perror("semop") ;
}

void down(int semid , int subsemid)
{
    struct sembuf sop;
    sop.sem_num = subsemid;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if(semop(semid , &sop , 1) < 0 )
        perror("semop") ;
} 

struct message
{
	long mtype ;
	char mtext[MAX_BUFFER_SIZE] ;
} ;

void atm_connect(int * , int * , int *) ;
void message_send(int , struct message * ) ;
void message_receive(int , struct message * , int) ;

int main()
{
    srand((unsigned int) getpid()) ;

	int atm_id , semaphore_id , message_queue_id ;
    int choice ;

    struct message buffer ;

    while(1)
    {
        sleep(1) ;
        printf("\n\nPress 1 to enter an ATM \nAny other key to exit\n\nEnter your choice : ") ;
        scanf("%d" , &choice) ;
        if(choice != 1) break ;
        atm_connect(&atm_id , &semaphore_id , &message_queue_id) ;
        //Joining protocol
        printf("Account Number : %d\n",getpid()) ;
        memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
        sprintf(buffer.mtext , "%d" , getpid() ) ;
        buffer.mtype = atm_id ;
        message_send(message_queue_id , &buffer) ;
        memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
        message_receive(message_queue_id ,  &buffer , atm_id+100) ;
        printf("%s\n" , buffer.mtext) ;

        int action ;
        while(1)
        {
            sleep(1) ;
            printf("\n\nPress 1 to Deposit money\nPress 2 to Withdraw \nPress 3 to view balance \nPress 4 to leave\n\nEnter your choice : ") ;
            scanf("%d" , &action) ;
            assert(action >=1 && action <=4) ;
            
            //send the choice to atm
            memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
            sprintf(buffer.mtext , "%d" , action ) ;
            buffer.mtype = atm_id ;
            message_send(message_queue_id , &buffer) ;
            

            if(action == 1)
            {
                // deposit money
                int deposit ; 
                printf("\nEnter the amount you want to deposit : Rs. ") ;
                scanf("%d", &deposit) ;

                memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
                sprintf(buffer.mtext , "%d" , deposit ) ;
                buffer.mtype = atm_id ;
                message_send(message_queue_id , &buffer) ;
                
                message_receive(message_queue_id ,  &buffer , atm_id+100) ;
                printf("%s\n" , buffer.mtext) ;

            }
            else if(action == 2)
            {
                // withdraw money
                int take_out ;
                printf("\nEnter the amount of money you want to withdraw : Rs. ") ;
                scanf("%d",&take_out) ;
                memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
                sprintf(buffer.mtext , "%d" , take_out) ;
                buffer.mtype = atm_id ;
                message_send(message_queue_id , &buffer) ;

                message_receive(message_queue_id ,  &buffer , atm_id+100) ;
                printf("%s\n" , buffer.mtext) ;
            }
            else if(action == 3)
            {
                // view the balance
                message_receive(message_queue_id ,  &buffer , atm_id+100) ;
                printf("\nYour balance is Rs. %s\n" , buffer.mtext) ;
            }
            else 
            {
                printf("\nLeaving ATM-%d\n" , atm_id) ;
                break ;
            }

        }

        up(semaphore_id , mutex) ;

    }

}



void message_send(int id , struct message * buffer)
{
    int result = msgsnd(id , buffer , (size_t)(MAX_BUFFER_SIZE) , 0 ) ;
    if(result == -1)
    {
        perror("Error in msg sending ") ;
        exit(1) ;
    }
}

void message_receive(int id , struct message * buffer , int atm_id)
{
    memset(buffer->mtext, 0 , MAX_BUFFER_SIZE) ;
    int result = msgrcv(id , buffer ,(size_t)(MAX_BUFFER_SIZE) , atm_id , 0) ;
    if(result < 0)
    {
        perror("Reading me load") ;
        exit(1) ;
    }
}

void atm_connect(int * atm_id , int * semaphore_id , int *message_queue_id)
{
	FILE * fp ;
	key_t semaphore_key , message_queue_key ;
	fp = fopen("ATM_LOCATER_FILE.txt","r+") ;
	char line[1000] ;
    struct message buffer ;
     int i = 1 ;
	while(fgets(line , 1000 , fp))
	{
		//printf("here \n") ;
        printf("Client enters ATM%d\n",i) ;
        int temp ;
		sscanf(line , "%d %d %d ", &temp , &message_queue_key , &semaphore_key) ;
		//printf("%d %d %d \n" , atm_id , message_queue_key , semaphore_key) ;
		*atm_id = temp ;
        *semaphore_id = semget(semaphore_key , 1 , IPC_CREAT|0666) ;
        int mutex_value = semctl(*semaphore_id , mutex , GETVAL , 0) ;
        //printf("%d\n",d) ;
        if(mutex_value == 1)
        {
        	//printf("here in loop \n") ;
        	*message_queue_id = msgget(message_queue_key , IPC_CREAT|0666) ;
        	down(*semaphore_id , mutex) ;
            printf("Welcome client %d\n" , getpid()) ;       
        	return ;
        }
	}
     printf("All atms are busy\nBye\n") ;
     exit(1) ;
}