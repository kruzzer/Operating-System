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

#define mutex 0 
#define no_of_messages 1 
#define total_messages_inserted 2
#define total_messages_deleted 3

int matrix[2][10] ;

void update_matrix()
{
    char buffer[5000] ;
    buffer[0] = '\0' ; 
    char number[10] ; 
    int i , j;
    FILE *fp ;
    fp = fopen("Matrix.txt","w+") ;

    for(i = 0 ; i < 2 ; i++)
    {
        for(j = 0 ; j < 10 ; j++)
        {
          memset(number,0,10) ;
          sprintf(number,"%d ",matrix[i][j]);
          strcat(buffer,number) ;
        }
        memset(number,0,10) ;
        sprintf(number,"\n");
        strcat(buffer,number) ; 
        
    }
       fprintf(fp,"%s",buffer) ;  

    //fp = fopen("Matrix.txt","w+") ;  
    fclose(fp) ;    
}

void get_matrix()
{
    FILE *fp ;
    fp = fopen("Matrix.txt","r+") ;
    int i, j;

    for(i = 0 ; i < 2 ; i++)
    {
        for(j = 0 ; j < 10 ; j++)
        {
            fscanf(fp, "%d ", &matrix[i][j]);
        }
    }
   
   fclose(fp) ;
}

void print_matrix()
{
    printf("\t\tThe Matrix File\n") ;
    int i , j ;
    for(i = 0 ; i < 2 ; i++)
    {
        for(j = 0 ; j < 10 ; j++)
        {
            printf("%d ",matrix[i][j]) ;
        }
        printf("\n");
    }
}

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
    char mtext[2] ;
} ;

int main(int argc , char * argv[])
{
    int producer_no = (int)(argv[1][0]) ;

    int msgid[2] ;
    
    key_t MSG_KEY_1 = 2323 ;
    key_t MSG_KEY_2 = 1919 ;

    msgid[0] = msgget(MSG_KEY_1 , IPC_CREAT|0666) ;
    msgid[1] = msgget(MSG_KEY_2 , IPC_CREAT|0666) ;

    int semid_queue[2] , file_write ;

    key_t QUEUE_KEY_1 = 2319 ;
    semid_queue[0] = semget(QUEUE_KEY_1 , 4 , IPC_CREAT|0666) ;

    key_t QUEUE_KEY_2 = 1923 ;
    semid_queue[1] = semget(QUEUE_KEY_2 , 4 , IPC_CREAT|0666) ;

    key_t QUEUE_KEY_3 = 2222 ;
    file_write = semget(QUEUE_KEY_3, 1 , IPC_CREAT|0666) ;

    srand((unsigned int)getpid() * 100);

    struct message buffer ;
    int flag = 1 , insert , queue_id ;

    while(1) 
    {
        if(flag == 1) 
        {
           insert = (rand()%1000)%50+1 ;
           queue_id = rand()%2 ;
           memset(buffer.mtext , 0 , 2) ;
           buffer.mtext[0] = (char)(insert) ;
           buffer.mtype = 1 ;
        }
        printf("Producer with id : %d trying to insert %d into queue with id : %d\n",producer_no,insert,queue_id) ;

        printf("Attempting to get control of the file\n") ;

        down(file_write , mutex) ;

        printf("Successful in getting contol of the file\n") ;

        get_matrix() ;
        matrix[queue_id][producer_no+5-1] = 1 ;
        update_matrix() ;
        print_matrix() ;
        up(file_write , mutex) ;

        printf("Giving up control of file\n") ;

        printf("Attempting down mutex\n") ;
            
        down(semid_queue[queue_id] , mutex) ;

        printf("Successful Down Mutex\n") ;
        
        printf("Attempting to get control of the file\n") ;
        down(file_write , mutex) ;
        printf("Successful in getting contol of the file\n") ;
        get_matrix() ;
        matrix[queue_id][producer_no+5-1] = 2 ;
        update_matrix() ;
        print_matrix() ;
        up(file_write , mutex) ;

        printf("Giving up control of file\n") ;

        int in_the_buffer = semctl(semid_queue[queue_id] , no_of_messages , GETVAL , 0) ;

        if(in_the_buffer < 10)
        {
           if(msgsnd(msgid[queue_id],&buffer,(size_t)(2),0) == -1)
           {
             perror("Error in msg sending ") ;
             exit(1) ;
           } 
           printf("Producer with id : %d inserted %d into queue with id : %d\n",producer_no,insert,queue_id) ;
           up(semid_queue[queue_id],no_of_messages) ;
           up(semid_queue[queue_id],total_messages_inserted) ;
           flag = 1 ;
        }

        else
        { 
          printf("The buffer is full\n") ;
          flag = 0 ;
        }
        
        printf("Attempting to get control of the file\n") ;
        down(file_write , mutex) ;
        printf("Successful in getting contol of the file\n") ;

        get_matrix() ;
        matrix[queue_id][producer_no+5-1] = 0 ;
        update_matrix() ;
        print_matrix() ;
        up(file_write , mutex) ;

        printf("Giving up control of file\n") ;

        up(semid_queue[queue_id] , mutex) ;

        printf("Giving up control of the queue with id : %d\n",queue_id) ;

        
        sleep(2) ;
    }

}