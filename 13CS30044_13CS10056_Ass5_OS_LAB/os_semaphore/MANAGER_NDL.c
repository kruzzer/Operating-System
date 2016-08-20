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

int main(int argc , char * argv[] )
{
    if(argc < 2)
    {
        printf("give the probability\n") ;
        exit(-1) ;
    }

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

    int i , j ;

    for(i = 0 ; i < 2 ; i++)
        for(j = 0 ; j < 10 ; j++)
            matrix[i][j] = 0 ;

    update_matrix() ;

    printf("/t/tThe Initial Matrix Values\n\n") ;

    print_matrix() ;

    semctl(semid_queue[0] , mutex , SETVAL , 1) ;
    semctl(semid_queue[0] , no_of_messages , SETVAL , 0) ;
    semctl(semid_queue[0] , total_messages_inserted , SETVAL , 0) ;
    semctl(semid_queue[0] , total_messages_deleted , SETVAL , 0) ;

    semctl(semid_queue[1] , mutex , SETVAL , 1) ;
    semctl(semid_queue[1] , no_of_messages , SETVAL , 0) ;
    semctl(semid_queue[1] , total_messages_inserted , SETVAL , 0) ;
    semctl(semid_queue[1] , total_messages_deleted , SETVAL , 0) ;
    
    semctl(file_write , mutex , SETVAL , 1) ;

    pid_t producer[5] , consumer[5];

    for(i = 1 ; i <= 5 ; i++)
    {
        char numberid[1] ;
        numberid[0] = (char)(i) ;
        producer[i-1] =fork() ;
        if(producer[i-1] == 0)
            execlp("xterm","xterm","-hold","-e","./producer",numberid,NULL) ;
    }

    char prob[10] ;
    strcpy(prob,argv[1]) ;

    for(i = 1 ; i <= 5 ; i++)
    {
        char numberid[1] ;
        numberid[0] = (char)(i) ;
        consumer[i-1] =fork() ;
        if(consumer[i-1] == 0)
            execlp("xterm","xterm","-hold","-e","./consumer_ndl",numberid,prob,NULL) ;  
    }


    while(1) 
    {
        printf("Entering into the while loop\n") ;

        down(file_write , mutex) ;
        get_matrix() ;
        up(file_write , mutex) ;

        print_matrix() ;

        printf("Checking for a loop\n") ;

        int queue_acquired[2] ;
        queue_acquired[0] = -5 ;
        queue_acquired[1] = -5 ;

        for(i = 0 ; i < 2 ; i++)
        {
            for(j = 0 ; j < 5 ; j++)
            {
                if(matrix[i][j] == 2)
                    queue_acquired[i] = j ;
            }
        }

        if(queue_acquired[0] >= 0 && queue_acquired[1] >= 0)
        {
            if(matrix[0][queue_acquired[1]] == 1 && matrix[1][queue_acquired[0]] == 1)
            {
                printf("Loop Detected \n") ;
                for(i = 0 ; i < 5 ; i++)
                    kill(producer[i],SIGSTOP) ;
                for(i = 0 ; i < 5 ; i++)
                    kill(consumer[i],SIGSTOP) ;
                

                if(queue_acquired[1] <= 4)
                {
                  printf("consumer[%d] ->",queue_acquired[1]+1) ;
                }
                else
                {
                 printf("producer[%d] ->",queue_acquired[1]+1-5) ;
                }
           
                printf("queue[0] ->") ;

                if(queue_acquired[0] <= 4)
                {
                  printf("consumer[%d] ->",queue_acquired[0]+1) ;
                }
                else
                {
                 printf("producer[%d] ->",queue_acquired[0]+1-5) ;
                }
           
                printf("queue[1] ->") ;

                if(queue_acquired[1] <= 4)
                {
                  printf("consumer[%d]",queue_acquired[1]+1) ;
                }
                else
                {
                 printf("producer[%d]",queue_acquired[1]+1-5) ;
                }

                FILE * result ;
                result = fopen("Result2.txt" , "a") ;
                char rr[5000] ;
                int a = semctl(semid_queue[0],total_messages_inserted,GETVAL,0)+semctl(semid_queue[1],total_messages_inserted,GETVAL,0) ;
                int b = semctl(semid_queue[0],total_messages_deleted,GETVAL,0)+semctl(semid_queue[1],total_messages_deleted,GETVAL,0) ;
                sprintf(rr,"With probability = %s , the total no. of messages inserted = %d and the total no. of messages deleted = %d\n",prob,a,b) ;
                fprintf(result, "%s",rr);
                fclose(result) ;

                







                exit(0) ;

            }

            else
            {
                printf("No loop Detected\n") ;
            }

        }
        else
        {
            printf("No chance of loop\n") ;
        }
        
        sleep(2) ; 
    }

}
