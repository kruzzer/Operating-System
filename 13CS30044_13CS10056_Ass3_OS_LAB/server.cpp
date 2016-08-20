
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAX_SIZE 100
#define MAX_MESSAGE_SIZE 1256
using namespace std;



struct message
{
    long mtype;
    char mtext[MAX_MESSAGE_SIZE];
};

int main()
{
	int msgid;
	int coupleNumber= 0;
	struct message buff,buff2,buff3;
	int len= 1000;
	key_t key = 6789;
	pid_t pid[MAX_SIZE];
	msgid = msgget(key,IPC_CREAT|0666);
	cout << "The message queue initiated with msgid = " << msgid << endl;
	for(;;)
	{
		int length;
		cout << "Waiting for Receiving " << endl;
		length = msgrcv(msgid,&buff,len,123,0);
		if(length==-1)
		{
			perror("msgrv failed\n");
        	exit(1);
		}
		//cout << length <<endl;
		buff.mtext[length]='\0';
		cout << "msg received " << buff.mtext<<endl;
		if(strcmp("couple",buff.mtext) == 0 || strcmp("Couple",buff.mtext) == 0 || strcmp("COUPLE",buff.mtext) == 0 )
		{
			//cout << "couple inside server" << endl;
			int length;
			length = msgrcv(msgid,&buff,len,123,0);
			if(length == -1)
			{
				perror("msgrv failed\n");
        		exit(1);
			}
			buff.mtext[length]='\0';
			pid_t pidNum = atoi(buff.mtext);
			memset(buff.mtext, 0, 1256);
			//cout << pidNum << endl;
			int breakerflag = 0;
			for(int i = 0;i<coupleNumber;i++)
			{
				if(pid[i] == pidNum)
				{
					breakerflag = 1;
					break;
				}
			}
			if(breakerflag == 1)
			{
				cout << "entered" << endl;
				cout <<buff.mtext << endl; 
				strcpy(buff.mtext,"Already Coupled");
				buff.mtype = pidNum;
				if(msgsnd(msgid,&buff,len,0)==-1) 
				{
					cout << "error " << endl;
					exit(1);
				}	
				memset(buff.mtext, 0, 1256);
				continue;
			}
			pid[coupleNumber] = pidNum;
			buff.mtype = pidNum;
			strcpy(buff.mtext,"Successful Couple");
			if(msgsnd(msgid,&buff,len,0)==-1) 
			{
				cout << "error" << endl;
				exit(1);
			}
			memset(buff.mtext, 0, 1256);
			coupleNumber = coupleNumber + 1;
			cout << "Broadcast Group Now is - " << endl;
			for(int i = 0;i<coupleNumber;i++)
			{
				cout << i+1 << " - " << pid[i] << endl;
			}
			cout << "BroadCast Group Updated Succesfully" <<endl;
		}
		else if(strcmp("uncouple",buff.mtext) == 0 || strcmp("Uncouple",buff.mtext) == 0 || strcmp("UNCOUPLE",buff.mtext) == 0 )
		{
			//cout << "uncouple inside server" << endl;
			int length;
			if(length = msgrcv(msgid,&buff,len,123,0)==-1)
			{
				perror("msgrv failed\n");
        		exit(1);
			}
			//cout << buff.mtext;
			pid_t pidNum = atoi(buff.mtext);
			//cout << " " << pidNum << endl;
			memset(buff.mtext, 0, 1256);
			buff.mtype = pidNum;
			pid_t pid2[MAX_SIZE];
			int j = 0;
			for(int i = 0;i<coupleNumber;i++)
			{
				if(pid[i] != pidNum)
				{
					pid2[j] = pid[i];
					j++;
				}
			}
			coupleNumber = coupleNumber - 1;
			for(int i = 0;i<j;i++)
			{
				pid[i] = pid2[i];
			}
			cout << "Broadcast Group Now is - " << endl;
			for(int i = 0;i<coupleNumber;i++)
			{
				cout << i+1 << " - " << pid[i] << endl;
			}
			strcpy(buff.mtext,"Successful Uncouple");
			if(msgsnd(msgid,&buff,len,0)==-1) 
			{
				cout << "error" << endl;
				exit(1);
			}
			memset(buff.mtext, 0, 1256);
			cout << "BroadCast Group Updated Succesfully" << endl;
		}
		else
		{
			char **args;
			args= (char**)malloc(100*sizeof(char*));
    		int pos=0;
    		char *elements;
    		char textInput2[100];
    		strcpy(textInput2,buff.mtext);
    		//cout <<"textInput2 - "<< textInput2 << endl;
    		elements = strtok(textInput2," \t\n");
   	 		while (elements != NULL)
    		{
        		args[pos] = elements;
        		pos++;
        		elements = strtok(NULL, " \t\n");
    		}
    		args[pos] = NULL;


    		//cout << args[0] << endl;
			memset(buff2.mtext, 0, 1256);
			int l1;
			char buff2text[1000];
			strcpy(buff2text,"");
			if( strcmp(args[0],"cd") != 0 && strcmp(args[0],"clear") !=0 )
			{
				//cout << "Enter without cd" << endl;
				l1 = msgrcv(msgid,&buff2,len,123,0); 
				if(l1 ==-1)
				{
					perror("msgrv failed\n");
        			exit(1);
				}
				buff2.mtext[l1] == '\0';
				//cout << buff2.mtext << endl;
				strcpy(buff2text,buff2.mtext);
			}
			//cout << "Reached after cd and clear" << endl;
			if(msgrcv(msgid,&buff3,len,123,0)==-1)
			{
				perror("msgrv failed\n");
        		exit(1);
			}

			int pidNum = atoi(buff3.mtext);
			char sendText[1000];
			strcpy(sendText,"");
			strcpy(sendText,"\nTerminal ");
			stringstream strs;
  			strs << pidNum;
  			string temp_str = strs.str();
  			char* char_type = (char*) temp_str.c_str();
			strcat(sendText,char_type);
			strcat(sendText,":\n");
			//cout << buff.mtext << endl;
			strcat(sendText,buff.mtext);
			strcat(sendText,"\n");
			strcat(sendText,buff2text);
			strcpy(buff.mtext,sendText);
			//cout << buff.mtext << endl;
			//cout << "pidnum - " <<pidNum<< " coupelNum -" << coupleNumber<< endl;
			for(int i = 0;i<coupleNumber;i++)
			{
				//cout << "pidnum - " <<pid[i] << endl;
				if(pid[i] != pidNum)
				{
					buff.mtype = pid[i];
					cout << "Sending this - "<< buff.mtext << endl ;
					cout << "To the process " << pid[i] <<endl;
					int lengththis = strlen(buff.mtext);
					buff.mtext[lengththis-1] = '\0';
					if(msgsnd(msgid,&buff,strlen(buff.mtext),0)==-1)
					{
						cout << "Error" << endl;
						exit(1);
					}
				}
			}
			memset(buff2.mtext, 0, 1256);
			memset(buff.mtext, 0, 1256);		
		}
	}
	return 0;
}


