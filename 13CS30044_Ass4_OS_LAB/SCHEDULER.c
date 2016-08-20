#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#define MAX_MESSAGE_SIZE 1000
#define capacity 100

struct message 
{
	long mtype ;
	char mtext[MAX_MESSAGE_SIZE] ;
} ;

struct process_info
{
	pid_t  pid_number ;
	int  priority ;
	double response_time ;
	double wait_time ;
	double turn_around_time ;
	clock_t wait_start ;
	clock_t wait_end ;
	clock_t arrive ;
	clock_t terminate ;
	clock_t first_response ;
} ;

void reset(struct process_info * temp_process)
{
	temp_process->pid_number = -1 ;
    temp_process->priority = -1 ;
    temp_process->response_time = -1 ;
    temp_process->wait_time = 0 ;
    temp_process->turn_around_time = -1 ;
    temp_process->wait_start = -1 ;
    temp_process->wait_end = -1 ;
    temp_process->arrive = -1 ;
    temp_process->terminate = -1 ;
    temp_process->first_response = -1 ;
}

//helper
int get_Pid(char buff[])
{
	char num[100];
	int i = 0 ;
	while(buff[i] != '$')
	{
		num[i] = buff[i] ;
		i++ ;
	}
	num[i] = '\0' ;
	return atoi(num) ;
}

//helper function 
int get_Prior(char buff[])
{
	char num[100]; 
	int i = 0 , k = 0 ;
	while(buff[i] != '$') i++ ;
	i++ ;
	while(buff[i] != '$')
	{
		num[k] = buff[i] ;
		k++ ;
		i++ ;
	}
	num[k] = '\0' ;
	return atoi(num) ;

}

int main()
{
	int msgid , i ;
	key_t key = 2323 ;
  msgid = msgget(key,IPC_CREAT|0666) ;
  printf("The message queue id = %d\n",msgid) ;

 int choice ;
 printf("Press 1 for round robin\nPress 2 for priority based round robin\n\nEnter your choice : ") ;
 scanf("%d",&choice) ;   

if(choice == 1)
 {  
	 struct message buff_rec , buff_send ;
	 struct process_info run_queue[10] , wait_queue[10], terminate_queue[10] ;
	 int r_start = 0 , r_end = 0 , w_start = 0 , w_end = 0 , t_start = 0 , t_end = 0 ;

	 while(1)
	 {
		memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
		// IF a new process arrives 
		if(msgrcv(msgid,&buff_rec , MAX_MESSAGE_SIZE , 1919 , IPC_NOWAIT) > 0)
  	    {
          struct process_info temp_process ; 
          reset(&temp_process) ;
          temp_process.pid_number = get_Pid(buff_rec.mtext) ;
          temp_process.priority   = get_Prior(buff_rec.mtext) ;

          printf("Process with process id : %d and priority : %d recieved\n", temp_process.pid_number,temp_process.priority) ;
         // sleep(2) ;
          temp_process.arrive = clock() ;
          temp_process.wait_start = clock() ;
          run_queue[r_end] = temp_process ;
          r_end++ ;
     	}
       /*   
        if(r_start < r_end)
        {  
           printf("The PID in the running queue : ") ;
     	   for(i = r_start ; i < r_end ; i++)
     		  printf("%d\t",run_queue[i].pid_number) ;
     	   printf("\n") ;
     	  // sleep(3) ;
     	  }
       */

     	if(r_start < r_end)
     	{
     		//reset(temp_process) ;
     		struct process_info execute = run_queue[r_start] ;
     		for(i = r_start ; i < r_end-1 ; i++)
     		{
                 run_queue[i] = run_queue[i+1] ;  
     		}
     		r_end-- ;


           if(execute.first_response == -1)
   	        {
   	 	        execute.first_response = clock() ;
   	        }
   	      
   	      execute.wait_end = clock() ;
   	      execute.wait_time += (double)(execute.wait_end - execute.wait_start)  ;
          printf("Process id : %d is being scheduled \n" , execute.pid_number) ; 
         // sleep(2) ;
          if(kill(execute.pid_number , SIGUSR1) == 0 ) 
          	printf("Notify signal send to resume\n") ;
   	      else
   	       {
   	 	     perror("Load in notify signal ") ;
   	       }
         
          printf("Process with pid : %d is running\n",execute.pid_number ) ;

          
         int flag = 0 ; 

          for(i = 0 ; i < 1000 ; i++)
          {
            memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
            if(msgrcv(msgid,&buff_rec,MAX_MESSAGE_SIZE,execute.pid_number,IPC_NOWAIT)>0)
            {
            	if(buff_rec.mtext[0] == 'I')
            	{ 
            		flag = 1 ; 
                    printf("Process with pid : %d requested for I/0\n",execute.pid_number) ;
                   // sleep(2) ;
                    wait_queue[w_end] = execute ;
                    w_end++ ;
                    break ;
            	}
            	if(buff_rec.mtext[0] == 'T')
            	{
            		 flag = 1 ;
            		 printf("Process with pid : %d has terminated\n",execute.pid_number) ;
            		// sleep(2) ;
                 execute.terminate = clock() ;
                 execute.response_time = (double) (execute.first_response - execute.arrive)  ;
                 execute.turn_around_time = (double) (execute.terminate - execute.arrive) / CLOCKS_PER_SEC  ; 
            		 terminate_queue[t_end] = execute ;
            		 t_end++ ;

            		 break ;  
            	}
            }	
          }

          if(flag == 0)
          {
          	memset(buff_send.mtext , 0 , MAX_MESSAGE_SIZE) ;
          	strcpy(buff_send.mtext,"Suspend") ;
          	buff_send.mtype = execute.pid_number ;
          	if(msgsnd(msgid,&buff_send,(size_t)(MAX_MESSAGE_SIZE),0) == -1)
   	 	       {
   	 		       perror("Error in msg sending of Suspend signal") ;
               exit(1) ;
   	 	       }
            printf("Suspend signal send to Process with pid : %d\n",execute.pid_number) ; 
   	 	    execute.wait_start = clock() ; 
   	 	    run_queue[r_end] = execute ;
            r_end++ ;
          }
     	}

     	if(w_start < w_end)
     	{
     	  // printf("The PID in the waiting queue : ") ;
     	  //for(i = w_start ; i < w_end ; i++)
     		// printf("%d\t",wait_queue[i].pid_number) ;
     	  // printf("\n") ;
     	  // sleep(3) ;

     	   for(int i = w_start ; i < w_end ; i++)
     	   {
     	   	 memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
             if(msgrcv(msgid,&buff_rec, MAX_MESSAGE_SIZE , wait_queue[i].pid_number , IPC_NOWAIT) > 0)
             {
                if(buff_rec.mtext[0] == 'C')  
                {
                  printf("Process with pid : %d has completed I/O\n",wait_queue[i].pid_number) ;
                  //sleep(2) ;
                  wait_queue[i].wait_start = clock() ;
                  run_queue[r_end] = wait_queue[i] ;
                  r_end++ ;

                  int j ;
                  for(j = i ; j < w_end ; j++)
                  {
                    wait_queue[j] = wait_queue[j+1] ;
                  }
                  w_end-- ;
                  i-- ;
                }
             }
     	   }

     	}
     	
      if(t_end == 4) break ;
	 }
   
   double avg_response = 0 , avg_turnaround = 0 , avg_waiting = 0 ; 
   //printf("Do the house keeping stuff ") ;
   FILE *fp=fopen("result_RR.txt","w");
   for(i = 0 ; i < 4 ; i++)
   {
    fprintf(fp,"\t Process PID : %d\nPriority : %d\nTurn around time : %.2f sec\nWaiting time : %.2lf clock ticks\nResponse time : %.2lf clock ticks\n\n\n", terminate_queue[i].pid_number ,terminate_queue[i].priority ,terminate_queue[i].turn_around_time,terminate_queue[i].wait_time , terminate_queue[i].response_time);
    avg_response += terminate_queue[i].response_time ;
    avg_waiting  += terminate_queue[i].wait_time ;
    avg_turnaround += terminate_queue[i].turn_around_time ; 
   }
   
   fprintf(fp,"\n\n\t\t Average Values\nTurn around time : %.2f sec\nWaiting time : %.2lf clock ticks\nResponse time : %.2lf clock ticks\n",avg_turnaround/4,avg_waiting/4,avg_response/4);
   fclose(fp);
   return 0;

 }    

 else if(choice == 2)
 {
   struct message buff_rec , buff_send ;
   struct process_info run_queue[10] , wait_queue[10], terminate_queue[10] ;
   int r_start = 0 , r_end = 0 , w_start = 0 , w_end = 0 , t_start = 0 , t_end = 0 ;

   while(1)
   {
    memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
    // IF a new process arrives 
    if(msgrcv(msgid,&buff_rec , MAX_MESSAGE_SIZE , 1919 , IPC_NOWAIT) > 0)
        {
          struct process_info temp_process ; 
          reset(&temp_process) ;
          temp_process.pid_number = get_Pid(buff_rec.mtext) ;
          temp_process.priority   = get_Prior(buff_rec.mtext) ;

          printf("Process with process id : %d and priority : %d recieved\n", temp_process.pid_number,temp_process.priority) ;
         // sleep(2) ;
          temp_process.arrive = clock() ;
          temp_process.wait_start = clock() ;
          run_queue[r_end] = temp_process ;
          r_end++ ;
      }
          
       /*   
        if(r_start < r_end)
        {  
           printf("The PID in the running queue : ") ;
         for(i = r_start ; i < r_end ; i++)
          printf("%d\t",run_queue[i].pid_number) ;
         printf("\n") ;
        // sleep(3) ;
        }
      */
      if(r_start < r_end)
      {
        //reset(temp_process) ;
        struct process_info execute ;
        int maximum = 500;
        for(i = r_start ; i < r_end ; i++)
        {
              if(run_queue[i].priority < maximum)
              maximum = run_queue[i].priority ;  
        }
        for(i = r_start ; i < r_end ; i++)
        {
            if(run_queue[i].priority == maximum)
            {
              execute = run_queue[i] ;
              break ;
            }
        }        

        for( ; i < r_end-1 ; i++)
        {
                 run_queue[i] = run_queue[i+1] ;  
        }
        r_end-- ;


           if(execute.first_response == -1)
            {
              execute.first_response = clock() ;
            }
          
          execute.wait_end = clock() ;
          execute.wait_time +=  (double)(execute.wait_end - execute.wait_start)  ;
          printf("Process id : %d is being scheduled \n" , execute.pid_number) ; 
         // sleep(2) ;
          if(kill(execute.pid_number , SIGUSR1) == 0 ) 
            printf("Notify signal send to resume\n") ;
          else
           {
           perror("Load in notify signal ") ;
           }
         
          printf("Process with pid : %d is running\n",execute.pid_number ) ;

          
         int flag = 0 ; 

          for(i = 0 ; i < 2000 ; i++)
          {
            memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
            if(msgrcv(msgid,&buff_rec,MAX_MESSAGE_SIZE,execute.pid_number,IPC_NOWAIT)>0)
            {
              if(buff_rec.mtext[0] == 'I')
              { 
                flag = 1 ; 
                    printf("Process with pid : %d requested for I/0\n",execute.pid_number) ;
                   // sleep(2) ;
                    wait_queue[w_end] = execute ;
                    w_end++ ;
                    break ;
              }
              if(buff_rec.mtext[0] == 'T')
              {
                 flag = 1 ;
                 printf("Process with pid : %d has terminated\n",execute.pid_number) ;
                // sleep(2) ;
                 execute.terminate = clock() ;
                 execute.response_time = (double) (execute.first_response - execute.arrive)  ;
                 execute.turn_around_time = (double) (execute.terminate - execute.arrive) / CLOCKS_PER_SEC ; 
                 terminate_queue[t_end] = execute ;
                 t_end++ ;

                 break ;  
              }
            } 
          }

          if(flag == 0)
          {
            memset(buff_send.mtext , 0 , MAX_MESSAGE_SIZE) ;
            strcpy(buff_send.mtext,"Suspend") ;
            buff_send.mtype = execute.pid_number ;
            if(msgsnd(msgid,&buff_send,(size_t)(MAX_MESSAGE_SIZE),0) == -1)
             {
               perror("Error in msg sending of Suspend signal") ;
               exit(1) ;
             }
            printf("Suspend signal send to Process with pid : %d\n",execute.pid_number) ; 
          execute.wait_start = clock() ; 
          run_queue[r_end] = execute ;
            r_end++ ;
          }
      }

      if(w_start < w_end)
      {
        // printf("The PID in the waiting queue : ") ;
         //for(i = w_start ; i < w_end ; i++)
         // printf("%d\t",wait_queue[i].pid_number) ;
        // printf("\n") ;
        // sleep(3) ;

         for(int i = w_start ; i < w_end ; i++)
         {
           memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ;
             if(msgrcv(msgid,&buff_rec, MAX_MESSAGE_SIZE , wait_queue[i].pid_number , IPC_NOWAIT) > 0)
             {
                if(buff_rec.mtext[0] == 'C')  
                {
                  printf("Process with pid : %d has completed I/O\n",wait_queue[i].pid_number) ;
                  //sleep(2) ;
                  wait_queue[i].wait_start = clock() ;
                  run_queue[r_end] = wait_queue[i] ;
                  r_end++ ;

                  int j ;
                  for(j = i ; j < w_end ; j++)
                  {
                    wait_queue[j] = wait_queue[j+1] ;
                  }
                  w_end-- ;
                  i-- ;
                }
             }
         }

      }
      
      if(t_end == 4) break ;
   }
   
   double avg_response = 0 , avg_turnaround = 0 , avg_waiting = 0 ; 
   //printf("Do the house keeping stuff ") ;
   FILE *fp=fopen("result_PRR.txt","w");
  
   for(i = 0 ; i < 4 ; i++)
   {
    fprintf(fp,"\t Process PID : %d\nPriority : %d\nTurn around time : %.2f sec\nWaiting time : %.2lf clock ticks\nResponse time : %.2lf clock ticks\n\n\n", terminate_queue[i].pid_number ,terminate_queue[i].priority ,terminate_queue[i].turn_around_time,terminate_queue[i].wait_time , terminate_queue[i].response_time);
    avg_response += terminate_queue[i].response_time ;
    avg_waiting  += terminate_queue[i].wait_time ;
    avg_turnaround += terminate_queue[i].turn_around_time ; 
   }
   
   fprintf(fp,"\n\n\t\t Average Values\nTurn around time : %.2f sec\nWaiting time : %.2lf clock ticks\nResponse time : %.2lf clock ticks\n",avg_turnaround/4,avg_waiting/4,avg_response/4);
   fclose(fp);
   return 0;
 }
}

