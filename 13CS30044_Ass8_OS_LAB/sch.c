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

#define MAX_BUFFER_SIZE 100
#define FROM_PROCESS_TO_SCH 20
#define FROM_SCH_TO_PROCESS 50
#define FROM_MMU_TO_SCH 250
#define FROM_SCH_TO_MMU 310
#define FROM_PROCESS_TO_MMU 500
#define FROM_MMU_TO_PROCESS 600


struct message
{
	long mtype ;
	char mtext[MAX_BUFFER_SIZE] ;
} ;

void message_send(int id , struct message * buffer)
{
	int result = msgsnd(id , buffer , (size_t)(MAX_BUFFER_SIZE) , 0 ) ;
	if(result == -1)
	{
		perror("Error in msg sending ") ;
		exit(1) ;
	}
}

void message_receive(int id , struct message * buffer , int rec_type)
{
	memset(buffer->mtext, 0 , MAX_BUFFER_SIZE) ;
	int result = msgrcv(id , buffer ,(size_t)(MAX_BUFFER_SIZE) , (long)rec_type , 0) ;
	if(result < 0)
	{
		perror("Reading me load") ;
		exit(1) ;
	}
}


int main(int argc , char * argv[])
{
	key_t mq_1_key , mq_2_key ; 
	int k , i ;
	mq_1_key = atoi(argv[1]) ;
	mq_2_key = atoi(argv[2]) ;
	printf("%d %d\n" , mq_1_key , mq_2_key) ;

	int mq_1_id = msgget(mq_1_key , IPC_CREAT|0666) ;
	int mq_2_id = msgget(mq_2_key , IPC_CREAT|0666) ;

	struct message buffer ;
	message_receive(mq_2_id , &buffer , 900) ;
	k = atoi(buffer.mtext) ;
	printf("K received = %d\n",k) ;

	int run_queue[10] , wait_queue[10] , terminated_queue[10] ;
	int r_start = 0 , r_end = 0 , w_start = 0 , w_end = 0 , t_start = 0 , t_end = 0 ;

	while(1)
	{
		memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
		if(msgrcv(mq_1_id , &buffer , MAX_BUFFER_SIZE , FROM_PROCESS_TO_SCH ,IPC_NOWAIT) > 0)
		{
			int process_id = atoi(buffer.mtext) ;
			run_queue[r_end++] = process_id ;
			printf("Process Id received : %d\n" , process_id) ;
		}

		if(r_start < r_end)
		{
			int execute_process_id = run_queue[r_start] ;
			for(i = r_start ; i < r_end-1 ; i++)
				run_queue[i] = run_queue[i+1] ;

			r_end-- ;

			// Notify signal send to the process

			memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
			buffer.mtype = FROM_SCH_TO_PROCESS +execute_process_id ;
			strcpy(buffer.mtext , "NOTIFY") ;
			message_send(mq_1_id , &buffer) ;
			printf("Notify signal send to process:%d\n" , execute_process_id) ;
			memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
			int result  = msgrcv(mq_2_id , &buffer , MAX_BUFFER_SIZE , FROM_MMU_TO_SCH , 0) ;
			if(buffer.mtext[0] == 'P')
			{
				// page fault handled
				printf("Page fault occured in process:%d\n" , execute_process_id) ;
				run_queue[r_end] = execute_process_id ;
				r_end++ ;

			}
			else if(buffer.mtext[0] == 'T')
			{
				// handle termination
				printf("Process:%d terminated\n" , execute_process_id) ;
				terminated_queue[t_end] = execute_process_id ;
				t_end++ ;
			}
			else
			{
				printf("Wrong message received from MMU\n") ;
				exit(0) ;
			}
		}
		if(t_end == k)
			break ;
	}



	printf("All the processes has been terninated\n") ;
	
	printf("Scheduler terminating\n") ;
	exit(1) ;


}