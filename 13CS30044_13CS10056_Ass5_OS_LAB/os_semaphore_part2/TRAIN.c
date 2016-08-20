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


int main(int argc , char * argv[])
{

	int direction = atoi(argv[1]) ;
	char direction_name[10] , right_direction_name[10];
	if(direction == North)
	{ 
	 	strcpy(direction_name , "North") ; 
	 	strcpy(right_direction_name , "West") ; 
	}
	else if(direction == West) 
	{ 
		strcpy(direction_name , "West") ; 
		strcpy(right_direction_name , "South") ;
	}
	else if(direction == South) 
	{ 
		strcpy(direction_name , "South") ;
		strcpy(right_direction_name , "East") ; 
	}
	else if(direction == East) 
	{ 
		strcpy(direction_name , "East") ; 
		strcpy(right_direction_name , "North") ; 
	}

	
	
	int right_direction = (direction+1)%4 ;
	int train_no = atoi(argv[2]) ;

	

	int semid_queue[5] , file_write , msg_id;

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

    struct message buffer ; 
    memset(buffer.mtext,0,50) ; 

    int  tt = getpid() ;
    char ttt[100] ;
    buffer.mtype = train_no+1 ;
    sprintf(ttt,"%d",tt) ;
    strcpy(buffer.mtext ,ttt) ;
    //printf("%s %ld\n",buffer.mtext,buffer.mtype) ;
    //int temp = strlen(buffer.mtext) ;
    //buffer.mtext[temp] = '\0' ;
    

   if(msgsnd(msg_id , &buffer , 50 , 0) == -1)
    {
             perror("Error in msg sending ") ;
             exit(1) ;
    }
    
    sleep(1) ;

    printf("Train<pid:%d> %s train started\n" , getpid() , direction_name ) ;


    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][direction] = 1 ;
    printf("Train<pid:%d> requests the %s-Lock\n",getpid() ,direction_name) ;
    update_matrix() ;
    //print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;
    
    printf("attempting semqueue_id direction down \n") ;
    down(semid_queue[direction] , mutex) ;
    printf("semqueue_id direction down successful \n") ;
    
    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][direction] = 2 ;
    printf("Train<pid:%d> acquires the %s-Lock\n",getpid() ,direction_name) ;
    update_matrix() ;
   // print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;


    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][right_direction] = 1 ;
    printf("Train<pid:%d> requests the %s-Lock\n",getpid() ,right_direction_name) ;
    update_matrix() ;
   // print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;
    
    printf("attempting semqueue_id direction down \n") ;
    down(semid_queue[right_direction] , mutex) ;
    printf("semqueue_id direction down successful \n") ;


    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][right_direction] = 2 ;
    printf("Train<pid:%d> acquires the %s-Lock\n",getpid() ,right_direction_name) ;
    update_matrix() ;
   // print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;

    printf("Train<pid:%d> requests Juntion-Lock\n",getpid()) ;
    down(semid_queue[junction],mutex) ;
    printf("Train<pid:%d> acquires Juntion-Lock ; Passing Junction ;\n",getpid()) ;
    
    //sleep(2) ;
    sleep(4) ; // to show deadlock
    
    up(semid_queue[junction],mutex) ;
    printf("Train<pid:%d> releases Juntion-Lock \n",getpid()) ;
    
    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][right_direction] = 0 ;
    printf("Train<pid:%d> releases the %s-Lock\n",getpid() ,right_direction_name) ;
    update_matrix() ;
    //print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;

    up(semid_queue[right_direction] , mutex) ;


    printf("attempting file_write down \n") ;
    down(file_write , mutex) ;
    printf("file_write down successful \n") ;
    get_matrix() ;
    matrix[train_no][direction] = 0 ;
    printf("Train<pid:%d> releases the %s-Lock\n",getpid() ,direction_name) ;
    update_matrix() ;
   // print_matrix() ;
    up(file_write , mutex) ;
    printf("leaving control of file\n") ;

    up(semid_queue[direction] , mutex) ;


    printf("Train<pid:%d> %s train departed the junciton \n" , getpid() , direction_name ) ;
}