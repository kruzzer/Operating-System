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

struct atm_record 
{
	int atm_id ;
	key_t message_id ;
	key_t semaphore_id ;
	key_t shared_memory_id ;
} ;

struct atm_record atm[9] ;

void get_atm_records(int) ;
void printw(int) ;
int main(int argc , char * argv[])
{
   int ATM_ID = atoi(argv[1]) ;
   int n = atoi(argv[2]) ;
   get_atm_records(n) ;
   printf("ATM-ID : %d\n",ATM_ID) ;
   printf("N : %d\n",n) ;
   printw(n) ;

}

void get_atm_records(int n)
{
	FILE * fp ;
	int i ;
	fp = fopen("ATM_LOCATER_FILE.txt","r+") ;
	for(i = 0 ; i < n ; i++)
	{
       fscanf(fp , "%d ",&atm[i].atm_id) ;
       fscanf(fp , "%d ",&atm[i].message_id) ;
       fscanf(fp , "%d ",&atm[i].semaphore_id) ;
       fscanf(fp , "%d ",&atm[i].shared_memory_id) ;
	}
}

void printw(int n)
{
	int i ;
	for(i = 0 ; i < n ; i++)
	{
	   printf("%d \t ",atm[i].atm_id) ;
       printf("%d \t ",atm[i].message_id) ;
       printf("%d \t ",atm[i].semaphore_id) ;
       printf("%d \t\n ",atm[i].shared_memory_id) ;
	}
}