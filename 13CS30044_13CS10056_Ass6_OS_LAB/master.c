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
#define RECORD_NO 25
#define MAX_BUFFER_SIZE 1024

typedef struct record
{
   int account_number ;
   int balance ;
   time_t last_updated ;
} bank_record ;

struct atm_record 
{
	int atm_id ;
	key_t message_key ;
	key_t semaphore_key ;
	key_t shared_memory_key ;
} ;

struct deposit_money
{
	int money ;
	time_t tm ;
} ;

struct withdraw_money
{
	int money ;
	time_t tm ;
} ;


typedef struct client_record
{
	int account_number ;
	int balance ;
	struct deposit_money deposited[15] ;
	struct withdraw_money withdrawn[15] ;
	int total_deposited ;
	int total_withdrawn ;

} clients ;


struct message
{
	long mtype ;
	char mtext[MAX_BUFFER_SIZE] ;
} ;


struct atm_record atm[9] ;
bank_record records[RECORD_NO] ;


void create_atm(int) ;
void create_atm_locater(int) ;
void message_send(int , struct message * ) ;
void message_receive(int , struct message * , int) ;
void initiate_join_protocol(int , int , int , int * ) ;
void initiate_view(int , int , int , int  , int ) ;


int main(int argc , char * argv[])
{
	int  n  , number_of_records = 0 , size_of_record;
	printf("Enter the number of ATM processes you want to generate(maximum of 8) : ") ;
	scanf("%d",&n) ;
	assert( n <= 8) ;
	
	create_atm(n) ;
	create_atm_locater(n) ;
	key_t master_message_key = 2323 ; // to communicate with the atm processes
	int master_message_id = msgget(master_message_key , IPC_CREAT|0666) ;
	size_of_record = sizeof(bank_record) ;
	
	key_t shared = 6767 ;
	int memory_id = shmget(shared , size_of_record * RECORD_NO , IPC_CREAT|0666) ;
	

	for(int i = 0 ; i < n ; i++)
	{
		char param1[5] ;
		sprintf(param1 , "%d" , i+1) ;
		
		char param2[5] ;
		sprintf(param2 , "%d" , n) ;

		char param3[10] ;
		sprintf(param3 , "%d" , master_message_key) ;
		pid_t pid = fork() ;
		if(pid == 0)
			execlp("xterm" , "xterm" , "-hold" , "-e" , "./banker" , param1 , param2 , param3 , NULL) ;

	}

	struct message buffer ;


	while(1)
	{
		message_receive(master_message_id , &buffer , 0) ;
		printf("Message received : %s\n" , buffer.mtext) ;
		printf("Message received from ATM : %ld\n", buffer.mtype ) ;
		if(buffer.mtext[0] == 'J')
		{
			initiate_join_protocol(memory_id , (int)(buffer.mtype) , master_message_id , &number_of_records) ;
		}

		else if(buffer.mtext[0] == 'V')
		{
			initiate_view(memory_id , (int)(buffer.mtype) , master_message_id , number_of_records , n) ;
		}


	}

	return 0 ;
}

void initiate_view(int memory_id , int atm_id , int master_message_id , int number_of_records , int atm_num)
{
	int i ;
	struct message buffer ;
	message_receive(master_message_id , &buffer , atm_id) ;
	printf("Account no received : %s\n" , buffer.mtext) ;
	int acc_no = atoi(buffer.mtext) ;
	bank_record * ptr ;
	ptr = shmat(memory_id , NULL , 0) ;
	int n = number_of_records ;
	int client_id ;
	for(i = 0 ; i < n ; i++)
	{
		if(acc_no == ptr[i].account_number)
		{
			printf("Account Exists in view\n") ;
			client_id = i ;
			break ;
		}
	}
	
	int dep = 0 , with = 0 ;
	int p ; 
	for(p = 0 ; p < atm_num ; p++)
	{
	  //printf("No:%d\n",p) ;  
	  clients * pttr ;
	  int client_mem_id = shmget(atm[p].shared_memory_key , sizeof(clients)*25 , IPC_CREAT|0666) ;
	  
	  pttr = shmat(client_mem_id , NULL , 0) ;

	  for(i = 0 ; i < 25 ; i++)
	  {
		if(ptr[client_id].account_number == pttr[i].account_number)
		{
			//printf("Match found with ATM-%d\n",p+1) ;
			dep = dep + pttr[i].total_deposited ;
			with = with + pttr[i].total_withdrawn ;
			pttr[i].total_deposited = 0 ;
			pttr[i].total_withdrawn = 0 ;
			pttr[i].deposited[0].money = -1 ;
			pttr[i].withdrawn[0].money = -1 ;
		}
	  }
	  int lol = shmdt((clients *)pttr) ; 
	
	}

	ptr[client_id].balance = ptr[client_id].balance + dep - with ;
	ptr[client_id].last_updated = time(NULL) ;

	for(p = 0 ; p < atm_num ; p++)
	{
	  clients * pttr ;
	  int client_mem_id = shmget(atm[p].shared_memory_key , sizeof(clients)*25 , IPC_CREAT|0666) ;
	  pttr = shmat(client_mem_id , NULL , 0) ;

	  for(i = 0 ; i < 25 ; i++)
	  {
		if(ptr[client_id].account_number == pttr[i].account_number)
		{
		   pttr[i].balance = ptr[client_id].balance ;
		}
	  }
	  int lol = shmdt((clients *)pttr) ; 
	
	}

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	strcpy(buffer.mtext , "Done") ;
	buffer.mtype = atm_id + 100 ;
	message_send(master_message_id , &buffer ) ;

	int kl = shmdt((bank_record *)ptr) ;
}

void initiate_join_protocol(int memory_id , int atm_id , int master_message_id , int * number_of_records)
{
	bank_record * ptr ;
	int i , n ;

	struct message buffer ;
	//printf("yola\n") ;
	message_receive(master_message_id , &buffer , atm_id) ;
	//printf("yo\n") ;
	printf("Account no received : %s\n" , buffer.mtext) ;
	int acc_no = atoi(buffer.mtext) ;


	ptr = shmat(memory_id , NULL , 0) ;
	n = *number_of_records ; 
	for(i = 0 ; i < n ; i++)
	{
		if(acc_no == ptr[i].account_number)
		{
			printf("Account Exists\n") ;
			memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
			sprintf(buffer.mtext , "%d" , ptr[i].balance) ;
			buffer.mtype = atm_id+100 ;
			message_send(master_message_id , &buffer) ;
			int k = shmdt((bank_record *)ptr) ;
			return ;
		}
	}

	printf("Account does not exist . Create a new account \n") ;

	ptr[n].account_number = acc_no ;
	ptr[n].balance = 0 ;
	ptr[n].last_updated = time(NULL) ;
	*number_of_records = n+1 ;

	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	sprintf(buffer.mtext , "%d" , ptr[n].balance) ;
	buffer.mtype = atm_id +100;
	message_send(master_message_id , &buffer) ;
	int k = shmdt((bank_record *)ptr) ;
	return ;
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
	int result = msgrcv(id , buffer ,(size_t)(MAX_BUFFER_SIZE) , (long)atm_id , 0) ;
	if(result < 0)
	{
		perror("Reading me load") ;
		exit(1) ;
	}
}


void create_atm(int n)
{
	int i ; 
	for(i = 0 ; i < n ; i++)
	{
		atm[i].atm_id = (i+1) ;
		atm[i].message_key = (i+1)*10+2 ;
		atm[i].semaphore_key = (i+1)*10+3 ;
		atm[i].shared_memory_key = (i+1)*10+4 ; 
	}
}

void create_atm_locater(int n)
{
	FILE *fp ;
	int i ;
	fp = fopen("ATM_LOCATER_FILE.txt","w+") ;
	char buffer[1000] , number[10] ;
	for(i = 0 ; i < n ; i++)
	{
		memset(buffer , 0 , 1000) ;

		memset(number ,0 , 10) ;
		sprintf(number , "%d " , atm[i].atm_id) ;
		strcat(buffer , number);

		memset(number ,0 , 10) ;
		sprintf(number , "%d " , atm[i].message_key) ;
		strcat(buffer , number);

		memset(number ,0 , 10) ;
		sprintf(number , "%d " , atm[i].semaphore_key) ;
		strcat(buffer , number);

		memset(number ,0 , 10) ;
		sprintf(number , "%d " , atm[i].shared_memory_key) ;
		strcat(buffer , number);

		strcat(buffer , "\n") ;
		fprintf(fp, "%s",buffer);	
	} 
	
	fclose(fp) ;
}