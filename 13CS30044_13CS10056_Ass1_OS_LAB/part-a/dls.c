//--------------------------------------------
//Assignment 1-a                             -
//Sidhartha Satapathy - 13CS10056            -
//Ken Kumar - 13CS30044                      -
//--------------------------------------------



#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>

int a[400];                   // array to store the integers that are read from the file
char filename[500] ;         // character array to store the path of the test file (to be taken as input from the user)


int main() 
{
    int  i,status,num,leftstart,leftend,rightstart,rightend,start,end;
    pid_t pid;
    pid_t main_process = getpid();
    //scanf("%d",&n);
    // take the path of the test file as input
    scanf("%s\n",filename) ;
    FILE *in_file  = fopen(filename , "r"); // read only
    fflush(stdin) ; 

    // num stores the integer to be searched
    scanf("%d",&num) ;
    // printf("hi\n");

    int temp , n = 0 ; 
    //fscanf(in_file , "%1d ", &temp); 

    // reading the integers from the file into the array
    while(!feof(in_file))
    {
      n++ ;
      fscanf(in_file, "%d ", &temp) ;
      a[n] = temp ;
          
    }
   
    //printf("%d\n",n );

    start =1 ;
    end = n+1;
    //for(i=1;i<=n;i++)
    //{
    //scanf("%d",&a[i]);
    //}

    /*
     The leftstart and leftend takes cares of the left half of parent
      and similarly the right half as well . 
      Now fork a process and then check if we are in the child or parent , 
      if we are inside the child then we check if the size of the child is
      less than equal to 5 and then if that is the case we search there itself , 
      else we recurse on that half of the array. 
      if we are in the parent then we fork another child and then we do the same as above for the right half 
      if we are still inside the parent we wait for one of the child process to terminate and check the exit status from that child .
      in this program if the exit status is not 0 then that is the index where we have found the number. WEXITSTATUS is used to get the exitstatus from the
      child.
      in case the number is not present in the array then both the children eventually return a 0 and hence we can determine that it is not present in
      the array in which case we print not found .
      Once we return to the main process from any of the child process and we have 
      successfully found the index , then we send a signal to kill all the processes.
      
      
      Note : We have used 1-base indexing 
      Note : The variable-names are self explainatory 
     
    */
    while(1)
    {
    leftstart = start ;
        leftend = (start+end)/2;
        rightstart = leftend;
        rightend = end;
        pid = fork();
        if(pid == 0) 
        {
        if(leftend - leftstart<=5)
        {
          //printf("child id - %d parent id - %d\n",getpid(),getppid());
          printf("searching in start - %d and end - %d\n",leftstart,leftend);
        int j;
        for(j = leftstart;j<leftend;j++)
        {
          if(a[j]==num)
          {
            //printf("child id - %d parent id - %d\n",getpid(),getppid());  
            //printf("left found - %d \n",j);
            exit(j);
          }
        }
        exit(0);
        }
            else
        {
        start = leftstart;
        end = leftend;
        }
        } 
        else 
        {
          pid_t pid2;
        pid2 = fork();
        if(pid2 == 0)
        {
        if(rightend - rightstart<=5)
          {
            //printf("child id - %d parent id - %d\n",getpid(),getppid());
            printf("searching in start - %d and end - %d\n",rightstart,rightend);
          int j;
          for(j = rightstart;j<rightend;j++)
          {
            if(a[j]==num)
            {
              //printf("child id - %d parent id - %d\n",getpid(),getppid());
              //printf("right found - %d \n",j);
              exit(j);
            }
          }
          exit(0);
          }
              else
          {
          start = rightstart;
          end = rightend;
          }
        }
        else
        {
          int k1,k2;
          waitpid(-1,&k1,0);
          if(WEXITSTATUS(k1) != 0)
          {
            //printf("i m here with child id - %d parent id - %d num - %d\n",getpid(),getppid(),WEXITSTATUS(k1));
            if(getpid() == main_process)
            {
              printf("main process - %d",main_process);
              printf("\nfound at - %d\n",WEXITSTATUS(k1));
              signal(SIGQUIT, SIG_IGN);
              kill(-main_process, SIGQUIT);
              exit(WEXITSTATUS(k1));
            }
            _exit(WEXITSTATUS(k1)); 
          }
          waitpid(-1,&k2,0);
          if(WEXITSTATUS(k2) != 0)
          {
            //printf("i m here with child id - %d parent id - %d num - %d\n",getpid(),getppid(),WEXITSTATUS(k2));
            if(getpid() == main_process)
            {
              printf("main process - %d",main_process);
              printf("\nfound at - %d\n",WEXITSTATUS(k2));
              signal(SIGQUIT, SIG_IGN);
              kill(-main_process, SIGQUIT);
              exit(WEXITSTATUS(k1));
            }         
            _exit(WEXITSTATUS(k2));
          }
          else
          {
            if(getpid() == main_process)
            {
              printf("main process - %d",main_process);
              printf("\nnot found\n");
              signal(SIGQUIT, SIG_IGN);
              kill(-main_process, SIGQUIT);
              exit(0);
            }           
            _exit(0);
          }
        }
            
        }
    }
    return 0;
    // code ends here
}