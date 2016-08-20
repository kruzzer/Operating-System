#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAX_MESSAGE_SIZE 1256
using namespace std;

struct message
{
    long mtype;
    char mtext[MAX_MESSAGE_SIZE];
};

pid_t mainProcessId;

int main()
{
	int msgid;
	struct message buff;
	
	int len= 500;
	key_t key = 6789;
	msgid = msgget(key,IPC_CREAT|0666);
	mainProcessId = getpid();
	char temp[100];
	char *dir;
	dir = (char *)malloc(100*sizeof(char));
   	( getcwd(temp, 100) ? std::string( temp ) : std::string("") );
	strcpy(dir,temp);

	pid_t pid1 = fork();
	if(pid1 == 0)
	{
		pid_t a =getpid();
		while(1)
		{
			struct message receivebuff; 
			if((msgrcv(msgid,&receivebuff,1000,getpid(),0))==-1)
			{
				perror("error - ");
				exit(1);
			}
			
			if(strcmp("Successful Couple",receivebuff.mtext) == 0)
			{
				cout << receivebuff.mtext << "\n" << "Your Group Id is - "<< a << endl;
			}
			else
			{
				cout << receivebuff.mtext <<endl;
			}
			memset(receivebuff.mtext, 0, 1256);
			
		}	
	}
	else
	{
		char *textInput;
		int lockOpen = 0;
		textInput = (char *)malloc(100*sizeof(char));
		for(;;)
		{
			sleep(1);
			cout << "Waiting for Input or other terminal" << endl;
			cout << dir << ">";
			fgets (textInput, 100, stdin);
			int lengthOfTextInput = strlen(textInput);
			textInput[lengthOfTextInput - 1] = '\0';
			//cout << textInput << endl;
			if(strcmp(textInput,"exit")==0)
			{
				signal(SIGQUIT, SIG_IGN);
	   			kill(-mainProcessId, SIGQUIT);
	   			exit(0);
			}
			else if(strcmp("couple",textInput) == 0 || strcmp("Couple",textInput) == 0 || strcmp("COUPLE",textInput) == 0)
			{
				//cout << "couple inside client" << endl;
				buff.mtype = 123;
				strcpy(buff.mtext,textInput);
				if(msgsnd(msgid,&buff,6,0)==-1)
				{
					perror("Error - ");
					exit(1);
				}
				buff.mtype = 123;
				memset(buff.mtext, 0, 1256);
				sprintf(buff.mtext,"%d",pid1);
				if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
				{
					perror("Error - ");
					exit(1);
				}
				memset(buff.mtext, 0, 1256);
				lockOpen=1;
			}
			else if(strcmp("uncouple",textInput) == 0 || strcmp("Uncouple",textInput) == 0 || strcmp("UNCOUPLE",textInput) == 0 )
			{
				//cout << "uncouple inside client" << endl;
				buff.mtype = 123;
				strcpy(buff.mtext,textInput);
				if(msgsnd(msgid,&buff,8,0)==-1)
				{
					perror("Error - ");
					exit(1);
				}
				buff.mtype = 123;
				memset(buff.mtext, 0, 1256);
				//cout << "1 - " << pid1;
				sprintf(buff.mtext,"%d",pid1);
				//cout << "2 - " << buff.mtext;
				if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
				{
					perror("Error - ");
					exit(1);
				}
				memset(buff.mtext, 0, 1256);
				lockOpen=0;
			}
			else
			{
				char **args;
				args= (char**)malloc(100*sizeof(char*));
    			int pos=0;
    			char *elements;
    			char textInput2[100];
    			strcpy(textInput2,textInput);
    			//cout <<"textInput2 - "<< textInput2 << endl;
    			elements = strtok(textInput2," \t\n");
   	 			while (elements != NULL)
    			{
        			args[pos] = elements;
        			pos++;
        			elements = strtok(NULL, " \t\n");
    			}
    			args[pos] = NULL;
    			//cout <<"before start- "<< textInput << endl;
				if(strcmp(args[0],"clear") == 0)
				{
					if(lockOpen)
					{
						buff.mtype=123;
						strcpy(buff.mtext,textInput);
						strcat(buff.mtext,"\n");
						if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
						{
							perror("error -");
						}
						FILE *in;
    					char buffstring[512];
    					//cout << "Command - "<<buff.mtext << endl;
    					if(!(in = popen(buff.mtext, "r")))
    					{
        					exit(1);
    					}
    					strcpy(buff.mtext,"");
 						while(fgets(buffstring, sizeof(buffstring), in)!=NULL)
 						{
 							cout << buffstring << endl;
    					}
					}
					else
					{
						FILE *in;
    					char buffstring[512];
    					//cout << "Command - "<<textInput << endl;
    					if(!(in = popen(textInput, "r")))
    					{
        					exit(1);
    					}
 						while(fgets(buffstring, sizeof(buffstring), in)!=NULL)
 						{
 							cout << buffstring << endl;
    					}
    					continue;
					}
					memset(buff.mtext, 0, 1256);
					buff.mtype=123;
					sprintf(buff.mtext,"%d",pid1);

					if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
					{
						perror("error:");
					}
					

					cout << "Sending From Process: " << buff.mtext << endl;
					memset(buff.mtext, 0, 1256);	

				}	
				else if(strcmp(args[0],"cd")==0)
				{
					if(chdir(args[1]))
					{
						cout << "Error - File Not Found";
						exit(1);
					}
					( getcwd(temp, 100) ? std::string( temp ) : std::string("") );
					strcpy(dir,temp);
					if(lockOpen)
					{
						//cout << "Inside lock in cd "<< endl;
						buff.mtype=123;
						strcpy(buff.mtext,textInput);
						strcat(buff.mtext,"\n");
						//cout << buff.mtext << endl;
						if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
						{
							perror("error -");
						}
					}
					else
					{
						continue;
					}
					
					
					memset(buff.mtext, 0, 1256);
					buff.mtype=123;
					sprintf(buff.mtext,"%d",pid1);
					if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
					{
						perror("error:");
					}
					cout << "Sending From Process: " << buff.mtext << endl;
					memset(buff.mtext, 0, 1256);
				}
				else
				{
					//cout << "except cd and clear" << endl;
					strcpy(buff.mtext,textInput);
					if(lockOpen)
					{
						buff.mtype=123;
						strcat(buff.mtext,"\n");
						if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
						{
							perror("error -");
						}
					}
					FILE *in;
    				char buffstring[512];
    				//cout << "Command - "<<buff.mtext << endl;
    				if(!(in = popen(buff.mtext, "r")))
    				{
        				exit(1);
    				}
    				strcpy(buff.mtext,"");
 					while(fgets(buffstring, sizeof(buffstring), in)!=NULL)
 					{
 						//cout << buffstring << endl;
				        strcat(buff.mtext,buffstring);
    				}
					//cout << "Reached Here" << endl;
					cout << "" << buff.mtext << endl;
					if(lockOpen)
					{
						buff.mtype=123;
						if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
						{
							perror("error -");
						}
						memset(buff.mtext, 0, 1256);
						buff.mtype=123;
						sprintf(buff.mtext,"%d",pid1);
						if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
						{
							perror("error -");
						}
						cout << "Sending From Process:" << buff.mtext << endl;
					}
					memset(buff.mtext, 0, 1256);
					
				}	
			

			}

		}
	}

	return 0;
}
