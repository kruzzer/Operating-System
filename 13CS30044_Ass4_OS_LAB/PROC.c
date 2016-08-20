#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define FROM_PROCESS_TO_SCH 20
#define FROM_SCH_TO_PROCESS 30
#define MAX_MESSAGE_SIZE 1000

double get_double(const char *str)
{
    /* First skip non-digit characters */
    /* Special case to handle negative numbers */
    while (*str && !(isdigit(*str) || ((*str == '-' || *str == '+') && isdigit(*(str + 1)))))
        str++;
 
    /* The parse to a double */
    return strtod(str, NULL);
}

int flag_pause ;
struct message 
{
  long mtype ;
  char mtext[MAX_MESSAGE_SIZE] ;
} ;

void notify()
{
  flag_pause = 1 ; 
  printf("Notify signal recieved from the scheduler\n") ;
}

int main(int argc , char* argv[])
{
  // get the agruments right
  int priority , t , sleep_time ;
  double sleep_prob ;
  srand((unsigned int)time(NULL));
  priority      = atoi(argv[1]) ; 
  t             = atoi(argv[2]) ;
  sleep_time    = atoi(argv[3]) ;
  sleep_prob = get_double(argv[4]) ;
  printf("priority recieved %d\n",priority) ;
  printf("no of iterations %d\n",t) ;
  printf("sleep time %d\n",sleep_time) ;
  printf("sleep probability %lf\n",sleep_prob) ;

  struct sigaction sighandle ;
  sighandle.sa_flags = SA_SIGINFO ;
  sighandle.sa_sigaction = &notify ;
  sigaction(SIGUSR1 , &sighandle, NULL) ;

  int msgid , i ;
  pid_t process_id ;
  struct message buff_rec , buff_send ;
  key_t key = 2323 ;
  msgid = msgget(key , IPC_CREAT|0666) ;
  process_id = getpid() ;
  printf("Process ID %d\n",process_id ) ;
  memset(buff_send.mtext, 0 ,MAX_MESSAGE_SIZE) ;
  sprintf(buff_send.mtext,"%d$%d$",process_id,priority) ;
  buff_send.mtype = 1919 ; 


  if(msgsnd(msgid , &buff_send , (size_t)(MAX_MESSAGE_SIZE) , 0 ) == -1)
  {
    perror("Error in msg sending of the pid_number ") ;
    exit(1) ;
  }

  int d ;
  
  flag_pause = 0 ;
  while(flag_pause == 0)
  {  
      d = pause() ;
  }

  for(i = 0 ; i < t ; i++)
  {
  	memset(buff_rec.mtext , 0 , MAX_MESSAGE_SIZE) ; 
  	if(msgrcv(msgid,&buff_rec,MAX_MESSAGE_SIZE,process_id,IPC_NOWAIT)>0)
    {
     if(buff_rec.mtext[0] == 'S')
      {
        printf("Suspend signal recieved \n") ;
        flag_pause = 0 ;
        while(flag_pause == 0)
          d =  pause() ;
      }
    }

      printf("PID : %d , Loop counter : %d\n",process_id , i+1 ) ;
      double r = rand()%100 + 1  ;
      r = r/100 ;
      int flag = 0 ;
      
      if(r < sleep_prob)
      {
        flag = 1 ;
        memset(buff_send.mtext, 0 ,MAX_MESSAGE_SIZE) ;
        strcpy(buff_send.mtext,"I/O Request") ;
        buff_send.mtype = process_id ;
        if(msgsnd(msgid , &buff_send , (size_t)(MAX_MESSAGE_SIZE) , 0 ) == -1)
         {
           perror("Error in msg sending of the IO request ") ;
           exit(1) ;
         }

        printf("PID : %d going for I/O \n", process_id) ;
        sleep(sleep_time) ;

        memset(buff_send.mtext, 0 ,MAX_MESSAGE_SIZE) ;
        strcpy(buff_send.mtext,"Completed I/O") ;
        buff_send.mtype = process_id ;
        if(msgsnd(msgid , &buff_send , (size_t)(MAX_MESSAGE_SIZE) , 0 ) == -1)
         {
           perror("Error in msg sending of the IO done signal ") ;
           exit(1) ;
         }

         printf("PID : %d came back from I/O \n", process_id) ;

      }

      if(flag == 1)
      {
        flag_pause = 0 ;
        while(flag_pause == 0)
          d =  pause() ;
      }
      

  }

  memset(buff_send.mtext, 0 ,MAX_MESSAGE_SIZE) ;
  strcpy(buff_send.mtext,"Terminate") ;
  buff_send.mtype = process_id ;
  if(msgsnd(msgid , &buff_send , (size_t)(MAX_MESSAGE_SIZE) , 0 ) == -1)
   {
      perror("Error in msg sending of the terminate signal ") ;
           exit(1) ;
   }
  printf("The process has terminated\n") ; 

}