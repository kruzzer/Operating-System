//--------------------------------------------
//Assignment 2-a                             -
//Sidhartha Satapathy - 13CS10056            -
//Ken Kumar - 13CS30044                      -
//--------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define MAX 40000

int primes[500000] ;
char AVAILABLE[] = "30001" ;
char BUSY[] = "30002" ;

// percomputing the primes and storing them in an array 

int PRIMES()
{
    int i , j ;
  for(i = 0 ; i < 500000 ; i++)
    primes[i] = 1 ;

  primes[0] = 0 ;
  primes[1] = 0 ;

  for(i = 2 ; i*i < 500000 ; i++ )
   {
     if(primes[i])
     {
        for(j = i*i ; j < 500000 ; j = j+i)
            primes[j] = 0 ;

     }
   }
}


 int main()
 {
    PRIMES() ;

       
    pid_t main_process , child_process[MAX] ;

    int main_to_child[MAX][2] ;
    int child_to_main[MAX][2] ;
    int i , j ; 
    char buffer[MAX] ;
    
    int child_marker , check ;

    int N , K ,size ;
    printf("Enter the value of N : ") ;
    scanf("%d",&N) ;
    printf("Enter the value of K : ") ;
    scanf("%d",&K) ;
    
    if(N > 2*K) 
        size = N ;
    else 
        size = 2*K ;
    int * primearr = (int *)malloc(sizeof(int)*size) ;
    int numprime = 0 ;
    srand(time(NULL));
   
    main_process = getpid() ;
    
    for( i = 0 ; i < K ; i++)
    {
        check = pipe(main_to_child[i]) ;
        if(check == -1)
        {
            perror("Not able to create Pipe...!") ;
            exit(0) ;
        }

        check = pipe(child_to_main[i]) ;
        if(check == -1)
        {
            perror("Not able to create Pipe...!") ;
            exit(0) ;
        }

        //close(main_to_child[2*i]);
        //close(child_to_main[2*i+1]);

        child_process[i] = fork() ;

        if(child_process[i] < 0)
        {
            perror("Not able to fork a child process...!") ;
            exit(0) ;
        }

        if(child_process[i] == 0)
        { 
           child_marker = i ;
            break ;
        }

    }
   
   if(getpid() == main_process)
    {
      //In the parent id
      for( i = 0 ; i < K ; i++)
      {
        close(main_to_child[i][0]);
        close(child_to_main[i][1]);
      }
        for(;;)
        {
            //printf("hi\n") ;
            for( i = 0 ; i < K ; i++)
            {
                //printf("hi1\n") ;
                  check = read(child_to_main[i][0] , buffer , MAX) ;
                  if(check == -1)
                  {
                    perror("Not able to read from the Pipe...!") ;
                    exit(0);
                  } 
                  //printf("hi2\n") ;

                  int sig = atoi(buffer) ;
                  //printf("sig %d\n",sig) ;
                  int caser = 1 ; 
                  switch(sig)
                    {
                     
                     //child is available
                    case 30001 :
                               
                                for(j = 0 ; j < K ; j++) 
                                {
                                    int random = (rand())%30000+1;
                                    //printf("random : %d\n",random) ; 
                                    sprintf(buffer , "%d" ,random) ;
                                    check = write(main_to_child[i][1] , buffer , MAX) ;
                                    if(check == -1)
                                    {
                                      perror("Not able to write into the pipe1...!") ;
                                      exit(0);
                                    } 
                                }
                                 break ;
                     // child is busy            
                    case 30002 :
                                 //printf("hi4\n") ;
                                 break ;
                    
                    // The child sends prime numbers to the parent
                    default :
                               // printf("hi5\n") ;
                                for(j = 0 ; j < numprime ; j++)
                                  {
                                    if(sig == primearr[j])
                                        caser = 0 ;
                                  }

                                
                                if(caser == 0) break ;
                               
                                 primearr[numprime++] = sig ;
                                 if(numprime == 2*K) 
                                 {
                                    int killer ;
                                    for(killer = 0 ; killer < K ; killer++)
                                    {
                                        check = kill(child_process[killer], SIGKILL) ;
                                        if(check == -1) 
                                            perror("") ;
                                    }
                                    printf("The prime numbers generated are : \n") ;
                                    for(j = 0 ; j < numprime ; j++)
                                        printf("%d\t",primearr[j]) ;
                                    free(primearr) ;
                                    printf("\n");
                                    exit(0) ;
                                 } 
                                 else 
                                 break ;                                   
                    }
            }
        }
    }

else 
    {
        //printf("hi6\n") ;
      // In the child process 
      close(main_to_child[child_marker][1]);
        close(child_to_main[child_marker][0]);
       int numbers[MAX] ; 
       for(;;)
       {
        // Sending available signal to the parent so that the parent can push random
        // integers into the pipe
        check = write(child_to_main[child_marker][1] , AVAILABLE , MAX) ;
        if(check == -1)
            {
             perror("Not able to write into the pipe2...!") ;
             exit(0);
            }
        
        //The child reads integers from the pipe 
        for(i = 0 ; i < K ; i++)
        {
            check = read(main_to_child[child_marker][0] , buffer , MAX ) ;
            if(check == -1)
            {
             perror("Not able to read into the pipe3...!") ;
             exit(0);
            }
            numbers[i] = atoi(buffer) ;
        } 

        //Sending Busy signal to the parent so that it doesn't push any more integers

        check = write(child_to_main[child_marker][1] , BUSY , MAX) ;
        if(check == -1)
            {
             perror("Not able to write into the pipe...!") ;
             exit(0);
            }


       // pushing the relevant prime numbers into the pipe
        for(i = 0 ; i < K ; i++)
        {
            
            if(primes[numbers[i]])
            {
                sprintf(buffer,"%d",numbers[i]);
                check = write(child_to_main[child_marker][1],buffer,MAX) ;
                if(check == -1)
                {
                  perror("Not able to write into the pipe...!") ;
                  exit(0);
                }
            }
        }    
       }    
    }
}
// code ends here