#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#define MAX_VERTICES 105
#define INF 10000007
#define mutex 0 
#define counter 1

int Graph[MAX_VERTICES][MAX_VERTICES] ;
int dist[MAX_VERTICES][MAX_VERTICES] ;

//int read_mutex , write_mutex ;
//key_t KEY1 , KEY2 ;
void init_graph_matrix(int * , int *) ;
void print_dst(int) ;
pthread_mutex_t read_mutex ;
pthread_mutex_t write_mutex ;
int read_count ;
//pthread_cond_t condition ;

struct parameters{
	int i ;
	int k ;
	int n ;
} ;


void * floyd_warshall(void * t){
	struct parameters * param = (struct parameters *)t ;
	int i , n , j  , k , result ;
	i = param->i ;
	k = param->k ;
	n = param->n ;
    //printf("k = %d , i = %d\n" , i , k ) ;
	for(j = 0 ; j < n ; j++){
        //down(read_mutex , mutex) ;
        pthread_mutex_lock(&read_mutex) ;
        //up(read_mutex , counter) ;
        read_count++ ;
        //getval = semctl(read_mutex , counter , GETVAL , 0) ;
        if(read_count == 1)
        	pthread_mutex_lock(&write_mutex) ;
        	//down(write_mutex , mutex) ;
        //up(read_mutex , mutex) ;
        pthread_mutex_unlock(&read_mutex) ; 
			result = dist[i][k] + dist[k][j] < dist[i][j] ;
		//down(read_mutex , mutex) ;
		pthread_mutex_lock(&read_mutex) ;	
		//down( read_mutex , counter) ;
		read_count-- ;
		//getval = semctl(read_mutex , counter , GETVAL , 0) ;
		if(read_count == 0)
			pthread_mutex_unlock(&write_mutex) ;
			//up(write_mutex , mutex) ;
		//up(read_mutex , mutex) ;
		pthread_mutex_unlock(&read_mutex) ;
		
		if(result == 1){
			pthread_mutex_lock(&write_mutex) ;
			//down(write_mutex , mutex) ;
				dist[i][j] = dist[i][k] + dist[k][j] ;
			//up(write_mutex , mutex) ;
			pthread_mutex_unlock(&write_mutex) ;	
		} 
	}   
	pthread_exit(NULL) ;
}

int main(){
	int n , m  , k , i , j ;
	
 	init_graph_matrix(&n , &m) ; 

	pthread_t threads[100] ;
	struct parameters t[100] ;
	pthread_attr_t attr ;
	pthread_mutex_init(&read_mutex , NULL) ;
	pthread_mutex_init(&write_mutex , NULL) ;
	//pthread_cond_init(&condition , NULL) ;
    
    //KEY1 = 1923 ;
    //read_mutex = semget(KEY1 , 2 , IPC_CREAT|0666) ;
    //KEY2 = 2323 ;
    //write_mutex = semget(KEY2 , 1 , IPC_CREAT|0666) ;

    //semctl(read_mutex , mutex , SETVAL , 1) ;
    //semctl(read_mutex , counter , SETVAL , 0) ;
    //semctl(write_mutex , mutex , SETVAL , 1) ;
  	read_count = 0 ;
	pthread_attr_init(&attr) ;
	pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_JOINABLE) ;

	for( k = 0 ; k < n ; k++){

		for( i = 0 ; i < n ; i++){
			// create threads ;
			struct parameters * param  = &(t[i]) ;
			param->i = i ;
			param->k = k ;
			param->n = n ;
			pthread_create(&threads[i] , &attr , floyd_warshall , (void *)param) ;
		}

		for(i = 0 ; i < n ; i++){
			// join the threads
			pthread_join(threads[i] , NULL) ;
		}
	}

	print_dst(n) ;
  
	pthread_attr_destroy(&attr) ;
	pthread_mutex_destroy(&read_mutex) ;
	pthread_mutex_destroy(&write_mutex) ;
	//pthread_cond_destroy(&condition) ;
	pthread_exit(NULL) ;
}

void print_dst(int n){
	printf("\nOutput:\n\n") ;
	int i , j ;
	for(i = 0 ; i < n ; i++){
		for(j = 0 ; j < n ; j++)
			if(dist[i][j] == INF)
				printf("INF\t") ;
			else
				printf("%d\t" , dist[i][j]) ;
		printf("\n") ;
	}
}

void init_graph_matrix(int *n , int *m){
	
	int i , j  , N , M , u , v , w ;

	printf("Enter the value of n : ") ;
	scanf("%d", n) ;

	printf("Enter the value of m : ") ;
	scanf("%d", m) ;

	N = *n ;
	M = *m ;

	assert( M <= N*(N+1)/2 ) ;

	for(i = 0 ; i < N ; i++)
		for(j = 0 ; j < N ; j++){
			Graph[i][j] = 0 ;
			if(i == j)
				dist[i][j] = 0 ;
			else
				dist[i][j] = INF ;
 		}
   
   printf("Enter %d sets of (u v w) values\n" , M) ;
	for(i = 0 ; i < M ; i++){
		scanf("%d %d %d" , &u , &v , &w) ;
		Graph[u-1][v-1] = 1 ;
		Graph[v-1][u-1] = 1 ;
		dist[u-1][v-1] = w ;
		dist[v-1][u-1] = w ;
	}
}
