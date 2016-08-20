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
};

//clients client[25] ;
struct atm_record atm[9] ;

void get_atm_records(int) ;
void print(int) ;
void message_send(int , struct message * ) ;
void message_receive(int , struct message * , int) ;
int get_client(int , int , int * , int , int) ;
int get_balance(int , int , int , int , int ) ;


int main(int argc , char * argv[])
{
	int ATM_ID , n , master_message_id , client_message_id , semaphore_id , memory_id ;
	int no_of_entries = 0 ;
	ATM_ID = atoi(argv[1]) ;
	n = atoi(argv[2]) ;
	key_t master_message_key = atoi(argv[3]) ;
	master_message_id = msgget(master_message_key , IPC_CREAT|0666) ;

	get_atm_records(n) ;

	key_t client_message_key = atm[ATM_ID-1].message_key ;
	client_message_id = msgget(client_message_key , IPC_CREAT|0666) ;

	key_t semaphore_key = atm[ATM_ID-1].semaphore_key ;
	semaphore_id = semget(semaphore_key , 1 , IPC_CREAT|0666) ;
    
    printf("Welcome to ATM-%d\n",ATM_ID) ;
	semctl(semaphore_id , mutex , SETVAL , 1) ;

	key_t memory_key = atm[ATM_ID-1].shared_memory_key ;
	memory_id = shmget(memory_key , sizeof(clients)*25 , IPC_CREAT|0666) ;
	
	struct message buffer ;

	while(1)
	{
		message_receive(client_message_id , &buffer , ATM_ID ) ;
		printf("client received with id %s \n" , buffer.mtext) ;
		int pos = get_client(atoi(buffer.mtext) , memory_id , &no_of_entries , master_message_id , ATM_ID) ;
		
		clients * ptr ;
		ptr = shmat(memory_id , NULL , 0) ;
        
        memset(buffer.mtext, 0 , MAX_BUFFER_SIZE) ;
        strcpy(buffer.mtext , "Successful Login") ;
        buffer.mtype = ATM_ID+100 ;
        message_send(client_message_id , &buffer) ;

        while(1)
        {
        	message_receive(client_message_id , &buffer , ATM_ID ) ;
        	int action = atoi(buffer.mtext) ;
        	printf("The choice received : %d\n" , action ) ;

        	if(action == 1)
            {
                // deposit money
                message_receive(client_message_id , &buffer , ATM_ID ) ;
        	    int deposit = atoi(buffer.mtext) ;
        	    memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
        	    int k = 0 ;
        	    while(ptr[pos].deposited[k].money != -1) k++ ;

        	    if(k >= 15)
        	    {
        	    	strcpy(buffer.mtext , "BufferOverflow . Cannot deposit . Please click view to sync\n") ;
        	    }
        	    else
        	    {
        	    	ptr[pos].deposited[k].money = deposit ;
        	    	ptr[pos].deposited[k].tm = time(NULL) ;
        	    	ptr[pos].total_deposited = ptr[pos].total_deposited + deposit ;
        	    	strcpy(buffer.mtext , "Successful deposit transaction\n") ;
        	    	k++ ;
        	    	if(k < 15)
        	    	{
        	    		ptr[pos].deposited[k].money = -1 ;
        	    	}
        	    }

        	    buffer.mtype = ATM_ID+100 ;
                message_send(client_message_id , &buffer) ;              
            }
            else if(action == 2)
            {
                // withdraw money
                message_receive(client_message_id , &buffer , ATM_ID ) ;
        	    int take_out = atoi(buffer.mtext) ;
        	    memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
        	    int k = 0 ;
        	    while(ptr[pos].withdrawn[k].money != -1) k++ ;

        	    if(k >= 15)
        	    {
        	    	strcpy(buffer.mtext , "BufferOverflow . Cannot withdraw . Please click view to sync\n") ;
        	    }
        	    else
        	    {
        	    	int p ;
        	    	int dep = 0 , with = 0 ;
        	    	for(p = 0 ; p < n ; p++)
        	    	{
        	    		clients * pttr ;
        	    		int client_mem_id = shmget(atm[p].shared_memory_key , sizeof(clients)*25 , IPC_CREAT|0666) ;
        	    		pttr = shmat(client_mem_id , NULL , 0) ;
        	    		int l ;
        	    		for(l = 0 ; l < 25 ; l++)
        	    		{
        	    			if(ptr[pos].account_number == pttr[l].account_number)
        	    			{
        	    				dep = dep + pttr[l].total_deposited ;
        	    				with = with + pttr[l].total_withdrawn ;
        	    			}
        	    		}
        	    		int lol = shmdt((clients *)pttr) ;
        	    	}

        	    	if(ptr[pos].balance + dep - with - take_out >= 0)
        	    	{
        	    		ptr[pos].withdrawn[k].money = take_out ;
        	    		ptr[pos].withdrawn[k].tm = time(NULL) ;
        	    		ptr[pos].total_withdrawn = ptr[pos].total_withdrawn + take_out ;
        	    		strcpy(buffer.mtext , "Successful Withdrawal transaction \n") ;
        	    		k++ ;
        	    		if(k < 15)
        	    		{
        	    			ptr[pos].withdrawn[k].money = -1 ;
        	    		}
        	    	}
        	    	else
        	    	{
        	    		strcpy(buffer.mtext , "Insufficient balance\n") ;
        	    	}
        	    }
        	    buffer.mtype = ATM_ID+100 ;
                message_send(client_message_id , &buffer) ;
            }
            else if(action == 3)
            {
                // view the balance
                memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
                strcpy(buffer.mtext , "View") ;
                buffer.mtype = ATM_ID ;
                message_send(master_message_id , &buffer) ;

                int bal = get_balance(ptr[pos].account_number , master_message_id, ATM_ID , no_of_entries , memory_id) ;
                
                memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
                sprintf(buffer.mtext , "%d" , bal) ;
                buffer.mtype = ATM_ID +100;
                message_send(client_message_id , &buffer) ;
            }
            else 
            {
                printf("CLient %d is leaving the atm \n",ptr[pos].account_number) ;
            	int k = shmdt((clients *)ptr) ;
                break ;
            }
        }  
	}

}


int get_balance(int acc_no , int master_message_id , int atm_id , int n , int memory_id)
{
	struct message  buffer ;
	memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
	sprintf(buffer.mtext , "%d" , acc_no) ;
    buffer.mtype = atm_id ;
	message_send(master_message_id , &buffer) ;
	message_receive(master_message_id , &buffer , atm_id+100) ;
	int i ;
	clients * ptr ;
	ptr = shmat(memory_id , NULL , 0 ) ;
	int return_value = -1;
    for(i = 0 ; i < n ; i++)
    {
    	if(acc_no == ptr[i].account_number)
    	{
    		return_value = ptr[i].balance ;
    	}
    }
    return return_value ;
}



int get_client(int acc_no , int memory_id , int * no_of_entries , int master_message_id ,int ATM_ID)
{
	clients * ptr ;
	int i , n;
	n = *no_of_entries ;
	printf("no_of_entries : %d\n",n) ;
	ptr = shmat(memory_id , NULL , 0 ) ;
    for(i = 0 ; i < n ; i++)
    {
    	if(acc_no == ptr[i].account_number)
    	{
    		int k = shmdt((clients *)ptr) ;
    		return i ;
    	}
    }

    printf("Client %d does not exist in local shared memory\n" , acc_no) ;
    //printf("ATM - ID : %d\n",ATM_ID ) ;
    struct message  buffer ;
    memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
    strcpy(buffer.mtext , "Joining Protocol") ;
    buffer.mtype = ATM_ID ;
    message_send(master_message_id , &buffer) ;

    memset(buffer.mtext , 0 , MAX_BUFFER_SIZE) ;
    sprintf(buffer.mtext ,"%d",acc_no) ;
    buffer.mtype = ATM_ID ;
    message_send(master_message_id , &buffer) ;

    message_receive(master_message_id , &buffer , ATM_ID+100) ; 
     
    //printf("balance received : %d \n", atoi(buffer.mtext)) ; 
    ptr[n].account_number = acc_no ;
    ptr[n].balance = atoi(buffer.mtext) ;
    ptr[n].total_deposited = 0 ;
    ptr[n].total_withdrawn = 0 ;
    ptr[n].deposited[0].money = -1 ;
    ptr[n].withdrawn[0].money = -1 ;

    *no_of_entries = n+1 ;
    //printf("yeaaagg\n") ;
    int k = shmdt((clients *)ptr) ;
    return n ;
    //

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

void get_atm_records(int n)
{
	FILE * fp ;
	int i ;
	fp = fopen("ATM_LOCATER_FILE.txt","r+") ;
	for(i = 0 ; i < n ; i++)
	{
	   fscanf(fp , "%d ",&atm[i].atm_id) ;
	   fscanf(fp , "%d ",&atm[i].message_key) ;
	   fscanf(fp , "%d ",&atm[i].semaphore_key) ;
	   fscanf(fp , "%d ",&atm[i].shared_memory_key) ;
	}
	fclose(fp) ;
}
/*void print(int n)
{
	int i ;
	for(i = 0 ; i < n ; i++)
	{
	   printf("%d \t ",atm[i].atm_id) ;
	   printf("%d \t ",atm[i].message_id) ;
	   printf("%d \t ",atm[i].semaphore_id) ;
	   printf("%d \t\n",atm[i].shared_memory_id) ;
	}
}*/