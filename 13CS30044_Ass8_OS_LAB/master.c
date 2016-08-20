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

struct message
{
	long mtype ;
	char mtext[MAX_BUFFER_SIZE] ;
} ;

typedef struct entry
{
	int frame ;
	int bit ;
} page_table_entry ;

typedef struct FRAME
{
	int process_id ;
	int page_number ;
	time_t TIME ;
} frame_info ;

int page_numbers[30] ;
key_t page_memory_key[30] ;
int page_memory_id[30] ;
char process_string[30][1000] = {0};
pid_t processes[120] ;

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
	srand((unsigned int)getpid()) ;
	int k , m , f , i , x , j , pid ;
	struct message buffer ;

	// k is the number of processes 
	// m is the maximum number of pages required by a process
	// f is the total number of frames
	printf("Enter the value of k (between one to ten): ") ;
	scanf("%d" , &k) ;
	assert(k >= 1 && k <= 10) ;

	printf("Enter the value of m (between five and thirty): ") ;
	scanf("%d" , &m) ;
	assert(m >= 5 && m <= 30) ;

	printf("Enter the value of f(between ten and fifty) : ") ;
	scanf("%d" , &f) ;
	assert(f >= 10 && f <= 50) ;

	for(i = 0 ; i < k ; i++)
	{
		page_numbers[i] = (rand()%2343543)%m+1 ;
		page_memory_key[i] = (i+1)*2+1020 ;
		page_memory_id[i] = shmget(page_memory_key[i] , sizeof(page_table_entry)*m , IPC_CREAT|0666) ;
		// initialize everything to -1
		page_table_entry *ptr ;
		ptr = shmat(page_memory_id[i] , NULL , 0) ;
		int random ;
		for(random = 0 ; random < m ; random++)
		{
			ptr[random].frame = -1 ;
			ptr[random].bit = -1 ;
		}

		random = shmdt((page_table_entry *)ptr) ;
		
		x  = rand()%(8*page_numbers[i] + 1) + 2*page_numbers[i] ;
		printf("page number = %d x = %d\n" ,page_numbers[i] , x) ;
		char temp[500] = "\0" ;
		if(i < (80*k / 100) )
		{
			for(j = 0 ; j < x ; j++)
			{
				int t = rand()%page_numbers[i] ;
				char num[50] ;
				sprintf(num , "%d&" , t) ;
				strcat(process_string[i] , num) ;
			}
			strcat(process_string[i] , "\0") ;
		}
		else
		{
			for(j = 0 ; j < x ; j++)
			{
				int t = rand()%(page_numbers[i] + 20) ;
				char num[50] ;
				sprintf(num , "%d&" , t) ;
				strcat(process_string[i] , num) ;
			}
			strcat(process_string[i] , "\0") ;

		}

		printf("%s\n" , process_string[i]) ;

		printf("%d %d\n" , page_numbers[i] , page_memory_key[i]) ;
	}

	key_t mq_1_key = 2000 ;
	int mq_1_id = msgget(mq_1_key , IPC_CREAT|0666) ;

	key_t mq_2_key = 2002 ;
	int mq_2_id = msgget(mq_2_key , IPC_CREAT|0666) ;

	key_t mq_3_key = 2004 ;
	int mq_3_id = msgget(mq_3_key , IPC_CREAT|0666) ;

	key_t frame_key = 3000 ;
	int frame_id = shmget(frame_key , sizeof(frame_info)*f , IPC_CREAT|0666) ;

	key_t master_to_mmu_key = 4000 ;
	int master_to_mmu_id = msgget(master_to_mmu_key , IPC_CREAT|0666) ;

	// forking the scheduler 
	pid = 0 ;
	// parameters for the sceduler process
	char param1[10] ;
	sprintf(param1 , "%d" , mq_1_key) ;
	char param2[10] ;
	sprintf(param2 , "%d" , mq_2_key) ;
	printf("sch %s %s \n" , param1 , param2) ;
	processes[pid] = fork() ;
	if(processes[pid] == 0)
		execlp("xterm" , "xterm" , "-hold" , "-e" , "./sch" , param1 , param2 , NULL) ;

	memset(param1 , 0 , 10) ;
	memset(param2 , 0 , 10) ;
	char param3[10] ;
	char param4[10] ;
	sprintf(param1 , "%d" , mq_2_key) ;
	sprintf(param2 , "%d" , mq_3_key) ;
	sprintf(param3 , "%d" , master_to_mmu_key) ;
	sprintf(param4 , "%d" , frame_key) ;

	printf("MMU %s %s %s %s \n" , param1 , param2 , param3 , param4) ;
	pid++ ;
	processes[pid] = fork() ;
	if(processes[pid] == 0)
		execlp("xterm" , "xterm" , "-hold" , "-e" , "./mmu" , param1 , param2 , param3 , param4 , NULL) ;




	/*
	// send the value of k
	// and then all the values of number of pages in each process
	// and then the shared memory id
	*/


	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	buffer.mtype = 10 ;
	sprintf(buffer.mtext , "%d" , k) ;
	message_send(master_to_mmu_id , &buffer) ;

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	buffer.mtype = 10 ;
	sprintf(buffer.mtext , "%d" , m) ;
	message_send(master_to_mmu_id , &buffer) ;

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	buffer.mtype = 10 ;
	sprintf(buffer.mtext , "%d" , f) ;
	message_send(master_to_mmu_id , &buffer) ;


	for(i = 0 ; i < k ; i++)
	{
		memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
		buffer.mtype = 10 ;
		sprintf(buffer.mtext , "%d" , page_numbers[i]) ;
		message_send(master_to_mmu_id , &buffer) ;

		memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
		buffer.mtype = 10 ;
		sprintf(buffer.mtext , "%d" , page_memory_key[i]) ;
		message_send(master_to_mmu_id , &buffer) ;
	}

	for(i = 0 ; i < k ; i++)
	{
		pid++ ;
		char param5[1000] ;
		memset(param1 , 0 , 10) ;
		memset(param2 , 0 , 10) ;
		memset(param3 , 0 , 10) ;
		memset(param4 , 0 , 10) ;

		sprintf(param1 , "%d" , i+1) ;
		sprintf(param2 , "%d" , mq_1_key) ;
		sprintf(param3 , "%d" , mq_3_key) ;
		strcpy(param5 , process_string[i]) ;

		processes[pid] = fork() ; 

		if(processes[pid] == 0)
			execlp("xterm" , "xterm" , "-hold" , "-e" , "./proc" , param1 , param2 , param3 , param5 , NULL) ;

		usleep(250) ;			
	}



	message_receive(master_to_mmu_id , &buffer , 5) ;
	printf("%s\n" , buffer.mtext) ;

	int status ; 
	for(i = 0 ; i <= pid ; i++)
		waitpid(processes[i], &status , 0) ;

	

	printf("Terminating Master\n") ;
	exit(1) ;
}