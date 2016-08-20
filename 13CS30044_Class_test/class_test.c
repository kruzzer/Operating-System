#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define MAX_NODES 100
#define MAX_EDGES 5050
struct nodes{
	int random ;
	int k ;
	int READ[100] ;
	int WRITE[100] ;
} ;

int main(){
	srand((unsigned int)getpid()) ;
	int n , m , i , result , u , v ;
	pid_t process[MAX_NODES] ;
	int e1[MAX_EDGES][2] , e2[MAX_EDGES][2] ;
	struct nodes node[MAX_NODES] ;
	printf("Enter the value of n : ") ;
	scanf("%d" , &n) ;
	assert(n >= 1 && n <= 100) ;
	printf("Enter the value of m : ") ;
	scanf("%d" , &m) ;
	assert(m >=1 && m <= n*(n+1)/2) ;
	
	for(i = 0 ; i < n ; i++){
		node[i].k = 0 ;
		node[i].random = rand()%100 + 1 ;
		// int r ;
		// scanf("%d" , &r) ;
		// node[i].random = r ;
	}

	for(i = 0 ; i < m ; i++){
		scanf("%d %d" , &u , &v ) ;
		assert(u >= 1 && u <= n) ;
		assert(v >= 1 && v <= n) ;
		u-- ;
		v-- ;

		result = pipe(e1[i]) ;
		if(result == -1){
			perror("Not able to create e1") ;
			exit(1) ;
		}
		result = pipe(e2[i]) ;
		if(result == -1){
			perror("Not able to create e1") ;
			exit(1) ;
		}
		printf("(%d,%d) -> (%d,%d)\n" , u , v , node[u].random , node[v].random) ;
		node[u].READ[node[u].k] = e1[i][0] ;
		node[u].WRITE[node[u].k] = e2[i][1] ;
		node[v].READ[node[v].k] = e2[i][0] ;
		node[v].WRITE[node[v].k] = e1[i][1] ;
		node[u].k++ ;
		node[v].k++ ;
	}

	for(i = 0 ; i < n ; i++){
		int j ;
		char command[10];
		char *arglist[2*MAX_NODES+20] ;
		for(j = 0 ; j < 2*MAX_NODES+20 ; j++){
			arglist[j] = (char *)malloc(30*sizeof(char)) ;
			memset(arglist[j] , 0 , 30) ;
		}

		strcpy(command , "./a.out") ;
		strcpy(arglist[0] , "./a.out") ;
		sprintf(arglist[1] , "%d" , n) ;
		sprintf(arglist[2], "%d" , node[i].k) ;
		sprintf(arglist[3], "%d" , node[i].random) ;
		sprintf(arglist[4], "%d" , i ) ;

		int temp = 5 ;
		//printf("number : %d k = %d\n" , node[i].random , node[i].k) ;
		for(j = 0 ; j < node[i].k ; j++){
			sprintf(arglist[temp] , "%d" , node[i].READ[j]) ;
            temp++ ;			
		}
		for(j = 0 ; j < node[i].k ; j++){
			sprintf(arglist[temp] , "%d" , node[i].WRITE[j]) ;
            temp++ ;			
		}
		arglist[temp] = NULL ;
		//printf("temp = %d\n" , temp) ;
		process[i] = fork() ;
		if(process[i] == 0){
			//printf("Sucessful in entering child process\n") ;
			if(execvp(command ,arglist) == -1){		
				perror("execv me load") ;
				exit(1) ;               
			}                
		}
		for(j = 0 ; j < 2*MAX_NODES+20 ; j++)
			free(arglist[j]) ;
	}

	// wait for status from each
	int status ;
	int total = 0 ;
	for(i = 0 ; i < n ; i++)
		total += node[i].random ;

	total = total/n ;
	printf("Expected : %d\n" , total) ;
	// total = 0 ;

	sleep(3) ;
	int fr , asrt ;
	for(i = 0 ; i < n ; i++){
		waitpid(process[i] , &status , 0) ;
		printf("Received from node-%d : %d\n",i+1 , WEXITSTATUS(status)) ;	
		if(i == 0) {
			fr = WEXITSTATUS(status)  ;
			assert(fr == total) ;
		}
		else assert(fr == WEXITSTATUS(status)) ;	     
	}
	// total = total / n ;
	printf("Hence the mean received : %d\n" , fr ) ;
	return 0 ;
}