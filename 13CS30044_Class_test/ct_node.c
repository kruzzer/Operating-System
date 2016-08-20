#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/sem.h> 

#define MAX_NODES 100
#define MAX 20
int LIMIT = 55555 ;

int READ[100] ;
int WRITE[100] ;

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

struct num_rec{
	int id ;
	int number ;
} ;

int check(struct num_rec * array , int  n , int number , int id){
	int i ;
	for(i = 0 ; i < n ; i++){
		if(array[i].number == number && array[i].id == id)
		return 0 ;
	}
	return 1 ;
}

int main(int argc , char * argv[]){

	key_t key = 1923 ;
	int id = semget(key , 1 , IPC_CREAT|0666) ;
	semctl(id , 0 , SETVAL , 1) ;
	//printf("executed\n") ;
	int n , k , result , i , j , number , index ;
	n = atoi(argv[1]) ;
	k = atoi(argv[2]) ;
	number = atoi(argv[3]) ;
	index = atoi(argv[4])+1 ;
	result = 5 ;
	for(i = 0 ; i < k ; i++){
		READ[i] = atoi(argv[result]) ;
		result++ ;
	}
	for(i = 0 ; i < k ; i++){
		WRITE[i] = atoi(argv[result]) ;
		result++ ;
	}
	if(argv[i] == NULL) printf("Yes\n") ;

	for(i = 0 ; i < k ; i++){
		char temp[MAX] ;
		memset(temp , 0 , MAX) ;
		sprintf(temp , "%d" ,number) ;
		result = write(WRITE[i] , temp , MAX) ;
		if(result == -1){
			perror("Not able to WRITE into the pipe1...!") ;
			exit(0) ; 
		}

		memset(temp , 0 , MAX) ;
		sprintf(temp , "%d" ,index) ;
		result = write(WRITE[i] , temp , MAX) ;
		if(result == -1){
			perror("Not able to WRITE into the pipe2...!") ;
			exit(0) ; 
		}

		memset(temp , 0 , MAX) ;
		sprintf(temp , "%d" ,LIMIT) ;
		result = write(WRITE[i] , temp , MAX) ;
		if(result == -1){
			perror("Not able to WRITE into the pipe3...!") ;
			exit(0) ;
		}

		memset(temp , 0 , MAX) ;
		sprintf(temp , "%d" ,LIMIT) ;
		result = write(WRITE[i] , temp , MAX) ;
		if(result == -1){
			perror("Not able to WRITE into the pipe3...!") ;
			exit(0) ;
		}
	}

	struct num_rec rec[LIMIT] ;
	struct num_rec received[LIMIT] ;
	int t = 0 , total = 0 ;
	received[total].number = number ;
	received[total].id = index ;
	total++ ;
	for(i = 0 ; i < n ; i++){
		t = 0 ;
		for(j = 0 ; j < k ; j++){
			char buff[20] ;
			while(1){
				memset(buff , 0 , 20) ;
				result = read(READ[j] , buff , MAX) ;
				if(result == -1){
					perror("Not able to read from pipe3...!") ;
					exit(0) ;
				}
				int temp = atoi(buff) ;
				memset(buff , 0 , 20) ;
				result = read(READ[j] , buff , MAX) ;
				if(result == -1){
					perror("Not able to read from pipe3...!") ;
					exit(0) ;
				}
				int temp2 = atoi(buff) ;
				if(temp == LIMIT && temp2 == LIMIT) break ;
				int temp3 = check(received , total , temp , temp2) ;
				if(temp3 == 1){
					rec[t].id = temp2 ;
					received[total].id = temp2 ;
					rec[t].number = temp ;
					received[total].number = temp ;
					t++ ;
					total++ ;
				}
			}	
		}

		for(j = 0 ; j < k ; j++){
			char buff[20] ;
			int m ;
			for(m = 0 ; m < t ; m++){
				memset(buff , 0 , 20) ;
				sprintf(buff , "%d" , rec[m].number) ;
				result = write(WRITE[j] , buff , MAX) ;
				if(result == -1){
					perror("Not able to write from pipe1...!") ;
					exit(0) ;
				}
				memset(buff , 0 , 20) ;
				sprintf(buff , "%d" , rec[m].id) ;
				result = write(WRITE[j] , buff , MAX) ;
				if(result == -1){
					perror("Not able to write from pipe1...!") ;
					exit(0) ;
				}
			}
			memset(buff , 0 , 20) ;
			sprintf(buff , "%d" , LIMIT) ;
			result = write(WRITE[j] , buff , MAX) ;
			if(result == -1){
				perror("Not able to write from pipe2...!") ;
				exit(0) ;	
			}
			memset(buff , 0 , 20) ;
			sprintf(buff , "%d" , LIMIT) ;
			result = write(WRITE[j] , buff , MAX) ;
			if(result == -1){
				perror("Not able to write from pipe2...!") ;
				exit(0) ;	
			}
		}
	}

	int total_sum = 0 ;
		
	// for (i = 0; i < total; i++)
	// {
	// 	for (j = i + 1; j < total; j++)
	// 	{
	// 		if (received[i] > received[j])
	// 		{
	// 			int a =  received[i];
	// 			received[i] = received[j];
	// 			received[j] = a;
	// 		}
	// 	}
	// }

	// down(id ,0) ;
	// printf("Number %d : total : %d\n" , number ,total) ;
	// for(i = 0 ; i < total ; i++){
	// 	printf("Number : %d \t list : %d \n" , number , received[i].number) ;
	// }
	for(i = 0 ; i < total ; i++){
		total_sum += received[i].number ;
	}
	// up(id,0) ; 

	int status = (total_sum)/(total) ;
	exit(status) ;
}