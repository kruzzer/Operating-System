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


typedef struct entry
{
	int frame ;
	int bit ;
} page_table_entry ;


typedef struct typo
{
	int process_id ;
	int page_number ;
} entry ;

typedef struct FRAME
{
	int process_id ;
	int page_number ;
	time_t TIME ;
} frame_info ;


entry parse(char buffer[])
{
	int i , j ;
	entry ret ;
	int l = strlen(buffer) ;
	char num[20] ;
	j = 0 ;
	int p = 0 ;
	memset(num , 0 , 20) ;
	for(i = 0 ; i < l ; i++)
	{
		if(buffer[i] != '&')
			num[j++] = buffer[i] ;
		else
		{
			num[j] = '\0' ;
			if(p == 0)
			{
				ret.process_id = atoi(num) ;
				p = 1 ; 
			}
			else if(p == 1)
			{
				ret.page_number = atoi(num) ;
				break ;
			}

			memset(num , 0 , 20) ;
			j = 0 ;
		}
	}

	return ret ;
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

key_t page_memory_key[30] ;
int page_memory_id[30] ;
int page_numbers[30] ;
int number_of_frames_being_used ;

int handle_page_fault(int f , int frame_id , int process_id , int page_number)
{
	//reqire the value of f 
	//require frame id
	// process_id , page nmber
	FILE *fp ;
	fp = fopen("Result.txt" , "a+") ;
	fprintf(fp , "Page fault sequence - (process id , page number) : (%d , %d)\n" , process_id , page_number ) ;
	fclose(fp) ;
	printf("Page fault sequence - (process id , page number) : (%d , %d)\n" , process_id , page_number) ;
	frame_info *ptr ;
	int ret ;
	ptr = shmat(frame_id , NULL , 0) ;
	if(number_of_frames_being_used < f)
	{
		ptr[number_of_frames_being_used].process_id = process_id ;
		ptr[number_of_frames_being_used].page_number = page_number ;
		ptr[number_of_frames_being_used].TIME = time(NULL) ;
		//printf("In Handler function : process_id = %d - page_number = %d\n" , process_id, page_number) ;
		ret = number_of_frames_being_used ;
		number_of_frames_being_used++ ;
	}
	else if(number_of_frames_being_used == f)
	{
		// implement lru
		time_t MIN = ptr[0].TIME ;
		int pos = 0 ;
		int j ;
		for(j = 1 ; j < number_of_frames_being_used ; j++)
		{
			if(MIN > ptr[j].TIME)
			{
				MIN = ptr[j].TIME ;
				pos = j ;
			}
		}
		frame_info victim = ptr[pos] ;

		ptr[pos].process_id = process_id ;
		ptr[pos].page_number = page_number ;
		ptr[pos].TIME = time(NULL) ;
		
		page_table_entry * pttr ;
		pttr = shmat(page_memory_id[victim.process_id-1] , NULL , 0) ;
		pttr[victim.page_number].frame = -1 ;
		pttr[victim.page_number].bit = -1 ;
		ret = pos ;
		int lol = shmdt((page_table_entry *)pttr) ;
 	}

 	int lol = shmdt((frame_info *)ptr) ; 
	return ret ;

}

int main(int argc , char * argv[])
{
	key_t mq_2_key , mq_3_key , mmu_to_master , frame_key ;
	int mmu_to_master_id , k , i , m , f;
	int mq_2_id , mq_3_id ;

	mq_2_key = atoi(argv[1]) ;
	mq_3_key = atoi(argv[2]) ;
	mmu_to_master = atoi(argv[3]) ;
	frame_key = atoi(argv[4]) ;
	number_of_frames_being_used =  0 ;
	//printf("%d %d %d %d\n" , mq_2_key , mq_3_key , mmu_to_master , frame_key) ;
	mmu_to_master_id = msgget(mmu_to_master , IPC_CREAT|0666) ;
	struct message buffer ;
	message_receive(mmu_to_master_id , &buffer , 10) ;
	k = atoi(buffer.mtext) ;
	message_receive(mmu_to_master_id , &buffer , 10) ;
	m = atoi(buffer.mtext) ;
	message_receive(mmu_to_master_id , &buffer , 10) ;
	f = atoi(buffer.mtext) ;

	int frame_id = shmget(frame_key , sizeof(frame_info)*f , IPC_CREAT|0666) ;
	//printf("(k,m,f)->(%d,%d,%d)\n" , k,m,f) ;

	mq_2_id = msgget(mq_2_key , IPC_CREAT|0666) ;
	mq_3_id = msgget(mq_3_key , IPC_CREAT|0666) ;

	// printf("k = %d\n", k ) ;
	for(i = 0 ; i < k ; i++)
	{
		message_receive(mmu_to_master_id , &buffer , 10) ;
		page_numbers[i] = atoi(buffer.mtext) ;
		message_receive(mmu_to_master_id , &buffer , 10) ;
		page_memory_key[i] = atoi(buffer.mtext) ;
		//printf("%d %d\n" , page_numbers[i] , page_memory_key[i]) ;
	}

	for(i = 0 ; i < k ; i++)
		page_memory_id[i] = shmget(page_memory_key[i] , sizeof(page_table_entry)*m , IPC_CREAT|0666) ;

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;

	sprintf(buffer.mtext , "%d" , k) ;
	buffer.mtype = 900 ;
	message_send(mq_2_id , &buffer) ;

	int process_counter = 0 ;
	int index_counter = 1 ;

	while(1)
	{
		message_receive(mq_3_id , &buffer , FROM_PROCESS_TO_MMU) ;
		entry received = parse(buffer.mtext) ;
		FILE *fp ;
		fp = fopen("Result.txt" , "a+") ;
		fprintf(fp , "Global odering - (index , process_id  , page number) : (%d , %d , %d)\n",index_counter , received.process_id , received.page_number) ;
		fclose(fp) ;
		printf("Global odering - (index , process_id  , page number) : (%d , %d , %d)\n",index_counter , received.process_id , received.page_number) ;
		index_counter++ ;
		int index = received.process_id - 1 ; // This is to access shared memory access 
		if(received.page_number == -9)
		{
			// process has terminated 
			// send appropriate msg to the scheduler
			printf("Process %d terminated\n" , received.process_id) ;
			memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
			buffer.mtype = FROM_MMU_TO_SCH ;
			strcpy(buffer.mtext , "TERMINATE") ;
			message_send(mq_2_id , &buffer) ;
			process_counter ++ ;
		}
		else if(page_numbers[index] <= received.page_number)
		{
			// illegal address 
			// ask process to terminate 
			// send msg to the scheduler
			FILE *fp ;
			fp = fopen("Result.txt" , "a+") ;
			fprintf(fp ,"Invalid page reference - (process id , page number) : (%d , %d)\n" , received.process_id , received.page_number) ;
			fclose(fp) ;

			printf("Invalid page reference - (process id , page number) : (%d , %d)\n" , received.process_id , received.page_number) ;
			memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
			buffer.mtype = FROM_MMU_TO_PROCESS+received.process_id ;
			int temp = -2 ;
			sprintf(buffer.mtext , "%d" ,temp) ;
			message_send(mq_3_id , &buffer) ;

			memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
			buffer.mtype = FROM_MMU_TO_SCH ;
			strcpy(buffer.mtext , "TERMINATE") ;
			message_send(mq_2_id , &buffer) ;
			process_counter++ ;
		}
		else
		{
			// check valid bit in the shared memory
			// do accordingly
			page_table_entry *ptr ;
			ptr = shmat(page_memory_id[index] , NULL , 0) ;
			if(ptr[received.page_number].bit == -1)
			{
				// handle page fault
				int pos = handle_page_fault(f,frame_id , received.process_id , received.page_number ) ;
				ptr[received.page_number].frame = pos ;
				ptr[received.page_number].bit = 1 ;

				memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
				buffer.mtype = FROM_MMU_TO_PROCESS+received.process_id  ;
				int minus_one = -1 ;
				sprintf(buffer.mtext , "%d" , minus_one) ;
				message_send(mq_3_id , &buffer) ;

				memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
				buffer.mtype = FROM_MMU_TO_SCH ;
				strcpy(buffer.mtext , "PAGE FAULT OCCURED") ;
				message_send(mq_2_id , &buffer) ;	
				int lol = shmdt((page_table_entry *)ptr) ;
			}
			else if(ptr[received.page_number].bit == 1)
			{
				frame_info *pttr ;
				pttr = shmat(frame_id , NULL , 0) ;

				if(pttr[ptr[received.page_number].frame].process_id == received.process_id )
				{
					//printf("Consistency check 1\n") ;
				}
				else
				{
					//printf("Not Consistent 1\n") ;
					//printf("frame_id %d - received id %d\n" , pttr[ptr[received.page_number].frame].process_id , received.process_id) ;
					exit(1) ;
				}

				pttr[ptr[received.page_number].frame].TIME = time(NULL) ;
				

				memset(buffer.mtext , 0  , MAX_BUFFER_SIZE) ;
				buffer.mtype = FROM_MMU_TO_PROCESS+received.process_id ; 
				sprintf(buffer.mtext , "%d" , ptr[received.page_number].frame) ;
				message_send(mq_3_id , &buffer) ;
			
				int lol = shmdt((frame_info *)pttr) ;
			}
			else
			{
				printf("MMU me load hai re dada bit - %d\n" , ptr[received.page_number].bit) ;
				exit(1) ;
			}

			
		}
		if(process_counter == k)
			break ;
	}


	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	buffer.mtype = 5 ;
	strcpy(buffer.mtext , "MMU terminated") ;
	message_send(mmu_to_master_id , &buffer) ;
	printf("MMU terminating\n") ;
	exit(1) ;
}
