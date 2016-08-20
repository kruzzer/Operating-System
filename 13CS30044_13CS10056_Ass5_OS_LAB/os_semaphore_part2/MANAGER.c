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

#define North 0
#define West  1 
#define South 2 
#define East 3
#define junction 4

#define mutex 0 

double probabilty = 0.7 ;


int matrix[200][4] ;

void update_matrix()
{
    char buffer[8000] ;
    buffer[0] = '\0' ; 
    char number[10] ; 
    int i , j;
    FILE *fp ;
    fp = fopen("Matrix.txt","w+") ;

    for(i = 0 ; i < 200 ; i++)
    {
        for(j = 0 ; j < 4 ; j++)
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

    for(i = 0 ; i < 200 ; i++)
    {
        for(j = 0 ; j < 4 ; j++)
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
    for(i = 0 ; i < 200 ; i++)
    {
        for(j = 0 ; j < 4 ; j++)
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
    char mtext[50] ;
} ;



int main( int argc , char * argv[] )
{
	char inputfile[200] , input_trains[200] ;
	if(argc < 3) 
	{
		printf("Please give filename as the input\n") ;
		exit(-1) ;
	}



	strcpy(inputfile , argv[1]) ;

	int semid_queue[5] , file_write , msg_id ;
    
    sscanf(argv[2],"%lf",&probabilty) ;
    //printf("prob : %lf\n",probabilty) ;

	key_t QUEUE_KEY_1 = 1919 ;
	semid_queue[North] = semget(QUEUE_KEY_1 , 1 , IPC_CREAT|0666) ;

	key_t QUEUE_KEY_2 = 2323 ;
	semid_queue[West] = semget(QUEUE_KEY_2 , 1 , IPC_CREAT|0666) ;

	key_t QUEUE_KEY_3 = 1923 ;
	semid_queue[South] = semget(QUEUE_KEY_3 , 1 , IPC_CREAT|0666) ;

	key_t QUEUE_KEY_4 = 2222 ;
	semid_queue[East] = semget(QUEUE_KEY_4 , 1 , IPC_CREAT|0666) ;

    key_t QUEUE_KEY_5 = 2322 ;
    semid_queue[junction] = semget(QUEUE_KEY_5 , 1 , IPC_CREAT|0666) ;

	key_t QUEUE_KEY_6 = 1922 ;
    file_write = semget(QUEUE_KEY_6 , 1 , IPC_CREAT|0666) ;

    key_t MSG_KEY_1 = 5000 ;
    msg_id = msgget(MSG_KEY_1 , IPC_CREAT|0666) ;
    
    semctl(semid_queue[North] , mutex , SETVAL , 1 ) ;
    semctl(semid_queue[West]  , mutex , SETVAL , 1 ) ;
    semctl(semid_queue[South] , mutex , SETVAL , 1 ) ;
    semctl(semid_queue[East]  , mutex , SETVAL , 1 ) ;
    semctl(semid_queue[junction] , mutex , SETVAL , 1) ;
    semctl(file_write , mutex , SETVAL , 1 ) ;

    FILE * fp ;
    fp = fopen(inputfile,"r") ;
    fscanf(fp,"%s",input_trains) ;
    printf("\n\tThe order in which to for trains : %s\n\n",input_trains) ;
    sleep(2) ;
    fclose(fp) ; 
    int total_no_of_trains = strlen(input_trains) ;
    int no_of_trains_forked = 0 ;

    int i , j ;

    for( i = 0 ; i < total_no_of_trains ; i++)
    {
        for(j = 0 ; j < 4 ; j++)

            matrix[i][j] = 0 ;
    }

    down(file_write,mutex) ;
    update_matrix() ;
    printf("Matrix Initialized\n") ;
   // print_matrix() ;
    up(file_write,mutex) ;


    
    srand((unsigned int)getpid() * 100) ;
   
    //int first_part = 1 ;
    

    pid_t train_processes[200]  ,train_processes_kill[200]; 

    while(no_of_trains_forked < total_no_of_trains)
    {
        double p =  rand()%101 ;
        p = p / 100 ;
        if(p <= probabilty)
        {
        	// create a train
        	int direction ;  
        	char t = input_trains[no_of_trains_forked] ;
        	if(t == 'N') direction = North ;
        	else if(t == 'W') direction = West ;
        	else if(t == 'S') direction = South ; 
        	else if(t == 'E') direction = East ;
            train_processes[no_of_trains_forked] = fork() ;
            if(train_processes[no_of_trains_forked] == 0)
            {
                // child process
                char param1[10] , param2[10];

                sprintf(param1,"%d",direction) ;
                sprintf(param2,"%d",no_of_trains_forked) ;
                execlp("xterm","xterm","-hold","-e","./TRAIN",param1,param2,NULL) ;
            }
            else 
            {
              //sleep(1) ;   
              struct message buffer ;
              memset(buffer.mtext,0,50) ;
             // printf("yola") ;
                if(msgrcv(msg_id,&buffer,(size_t)(50), no_of_trains_forked +1, 0) > 0)
                {
                   // printf("yola") ;
                    int receive = atoi(buffer.mtext) ;
                    train_processes_kill[no_of_trains_forked] = train_processes[no_of_trains_forked];
                    train_processes[no_of_trains_forked]  = receive ;
                }
 
        	  no_of_trains_forked++ ; // parent process
            }
        }
        else
        {
        	//check for deadlock
            down(file_write,mutex) ;
            get_matrix() ;
            //print_matrix() ;
            up(file_write,mutex) ;

            int acquired[4] ;
            acquired[0]  = -5 ;
            acquired[1]  = -5 ;
            acquired[2]  = -5 ;
            acquired[3]  = -5 ;

            for(j = 0 ; j < 4 ; j++)
            {
                for(i = 0 ; i < no_of_trains_forked ; i++)
                    if(matrix[i][j] == 2) 
                        {
                            acquired[j] = i ;
                            break ;
                        }
            }

            if(acquired[North] >= 0 && acquired[West] >= 0 && acquired[South] >= 0 && acquired[East] >=0 )
            {
               if(matrix[acquired[North]][West] == 1 && matrix[acquired[West]][South] == 1 && matrix[acquired[South]][East] == 1 && matrix[acquired[East]][North] == 1)
               {
                  printf("Deadlock Detected\n") ;
                  printf("Train<PID:%d> from North is waiting for Train<PID:%d> from West ---->",train_processes[acquired[North]] ,train_processes[acquired[West]] ) ;
                  printf("Train<PID:%d> from West is waiting for Train<PID:%d> from South ---->",train_processes[acquired[West]] ,train_processes[acquired[South]] ) ;
                  printf("Train<PID:%d> from South is waiting for Train<PID:%d> from East ---->",train_processes[acquired[South]] ,train_processes[acquired[East]] ) ;
                  printf("Train<PID:%d> from East is waiting for Train<PID:%d> from North   \n ",train_processes[acquired[East]] ,train_processes[acquired[North]] ) ;
                  
                  //for(i = 0 ; i < no_of_trains_forked ; i++)
                    //kill(train_processes_kill[i],SIGSTOP) ;
                while(1) ;
               }
            }
            else
            {
                printf("No chance of deadlock \n") ;
            }
        }
        sleep(1) ;
    }

    while(1)
    {

        down(file_write,mutex) ;
        get_matrix() ;
      //  print_matrix() ;
        up(file_write,mutex) ;

        int acquired[4] ;
        acquired[0]  = -5 ;
        acquired[1]  = -5 ;
        acquired[2]  = -5 ;
        acquired[3]  = -5 ;

        for(j = 0 ; j < 4 ; j++)
        {
            for(i = 0 ; i < no_of_trains_forked ; i++)
                if(matrix[i][j] == 2) 
                {
                    acquired[j] = i ;
                    break ;
                }
        }

        if(acquired[North] >= 0 && acquired[West] >= 0 && acquired[South] >= 0 && acquired[East] >= 0 )
        {
            if(matrix[acquired[North]][West] == 1 && matrix[acquired[West]][South] == 1 && matrix[acquired[South]][East] == 1 && matrix[acquired[East]][North] == 1)
            {
                printf("Deadlock Detected\n") ;
                printf("Train<PID:%d> from North is waiting for Train<PID:%d> from West ---->",train_processes[acquired[North]] ,train_processes[acquired[West]] ) ;
                printf("Train<PID:%d> from West is waiting for Train<PID:%d> from South ---->",train_processes[acquired[West]] ,train_processes[acquired[South]] ) ;
                printf("Train<PID:%d> from South is waiting for Train<PID:%d> from East ---->",train_processes[acquired[South]] ,train_processes[acquired[East]] ) ;
                printf("Train<PID:%d> from East is waiting for Train<PID:%d> from North   \n ",train_processes[acquired[East]] ,train_processes[acquired[North]] ) ;
                  
               // for(i = 0 ; i < no_of_trains_forked ; i++)
                  //kill(train_processes_kill[i],SIGSTOP) ;
                while(1) ;
            }
        }
        else
        {
            printf("No chance of deadlock \n") ;
        }


            sleep(1) ; 
    }

   return 0 ;
}
