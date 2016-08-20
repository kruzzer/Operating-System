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
#include <sys/ipc.h>
#include <sys/wait.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

int main()
{
	int i;  
    pid_t process[4] ;
	for(i = 0 ; i < 2 ; i++)
	{
		char priority[] = "10" ;
		char no_of_iterations[] = "10000" ;
		char sleep_time[] = "1" ;
		char sleep_prob[] = "0.3" ;
		process[i] = fork() ;
		if(process[i] == 0)
		{
           execlp("xterm","xterm","-hold","-e","./proc",priority,no_of_iterations,sleep_time,sleep_prob,NULL);
		}
	}

	for(    ; i < 4 ; i++)
	{
		char priority[] = "5" ;
		char no_of_iterations[] = "4000" ;
		char sleep_time[] = "3" ;
		char sleep_prob[] = "0.7" ;
		process[i] = fork() ;
		if(process[i] == 0)
		{
           execlp("xterm","xterm","-hold","-e","./proc",priority,no_of_iterations,sleep_time,sleep_prob,NULL);
		}
	}
}
