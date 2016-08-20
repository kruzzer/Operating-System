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


int page_numbers[500] ;
int counter ;

void get_list(char * msg)
{
	int i , j ;
	int l = strlen(msg) ;
	char num[20] ;
	j = 0 ;
	memset(num , 0 , 20) ;    
	for(i = 0 ; i < l ; i++)
	{
		if(msg[i] != '&')
			num[j++] = msg[i] ;
		else
		{
			num[j] = '\0' ;
			page_numbers[counter++] = atoi(num) ;
			memset(num , 0 , 20) ;
			j = 0 ;
		}

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

void go_to_wait(int id , int queue_id)
{
	struct message buffer ;
	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	message_receive(queue_id , &buffer , FROM_SCH_TO_PROCESS+id) ;
	if(buffer.mtext[0] == 'N')
		printf("Notify signal from scheduler received\n") ;
	else
	{
		printf("Wrong message from scheduler\n") ;
		exit(1) ;
	}

}

int main(int argc , char * argv[])
{
	int id , i;
	key_t mq_1_key , mq_3_key ;
	char process_string[1000] ;
	int mq_1_id , mq_3_id ;
	counter = 0 ;
	id = atoi(argv[1]) ;
	mq_1_key = atoi(argv[2]) ;
	mq_3_key = atoi(argv[3]) ;
	strcpy(process_string , argv[4]) ;
	get_list(process_string) ;

	mq_1_id = msgget(mq_1_key , IPC_CREAT|0666) ;
	mq_3_id = msgget(mq_3_key , IPC_CREAT|0666) ;

	printf("Process id = %d\n" , id) ; 
	// for(i = 0 ; i < counter ; i++)
	// {
	// 	printf("%d\n",page_numbers[i]) ;
	// }

	struct message buffer ;

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	sprintf(buffer.mtext , "%d" , id) ;
	buffer.mtype = FROM_PROCESS_TO_SCH ;
	message_send(mq_1_id , &buffer) ;

	go_to_wait(id , mq_1_id ) ;

	int index_number = 0 ;
	

	printf("Process scheduled for the first time\n") ;

	while(1)
	{
		// send page number to mmu
		printf("Requesting for %d page number\n", page_numbers[index_number]) ;
		memset(buffer.mtext , 0 ,MAX_BUFFER_SIZE) ;
		sprintf(buffer.mtext , "%d&%d&" , id , page_numbers[index_number]) ;
		buffer.mtype = FROM_PROCESS_TO_MMU ;
		message_send(mq_3_id , &buffer) ;
		message_receive(mq_3_id , &buffer , FROM_MMU_TO_PROCESS+id) ;
		int frame_number = atoi(buffer.mtext) ;
		
		if(frame_number >= 0)
		{
			printf("Frame number from MMU received : %d\n" , frame_number) ;
			index_number++ ;
		}

		else if(frame_number == -1)
		{
			printf("Page fault occured\n") ;
			go_to_wait(id , mq_1_id) ;
		}

		else if(frame_number == -2)
		{
			printf("Invalid page reference\n") ;
			exit(1) ;
		}	

		if(index_number == counter)
			break ;
	}

	printf("Process completed\n") ;
	memset(buffer.mtext , 0 ,MAX_BUFFER_SIZE) ;
	int temp = -9 ;
	sprintf(buffer.mtext , "%d&%d&" , id , temp) ;
	buffer.mtype = FROM_PROCESS_TO_MMU ;
	message_send(mq_3_id , &buffer) ;
	exit(1) ;
}