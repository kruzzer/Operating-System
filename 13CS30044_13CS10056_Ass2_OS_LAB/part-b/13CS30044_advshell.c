//--------------------------------------------
//Assignment 2-b                             -
//Sidhartha Satapathy - 13CS10056            -
//Ken Kumar - 13CS30044                      -
//--------------------------------------------


// including the relevant libraries to support our shell 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define max_dir 5000
#define max_cmd 500
#define max_arg 500
#define disk_blk_size 512
#define file_blk_size 1024
#define mode_MKDIR 0777

extern char **environ ;
// http://www.dauniv.ac.in/downloads/EmbsysRevEd_PPTs/Chap_5Lesson03EmsysNewQueuesPipesSockets.pdf
typedef struct pipe_queue
{
  char **queue_elements ;
  int number ;
}
 pipe_queue ;


  FILE *fp  , *fr ;
  
 int flag  ;

char** generate_args(char** arguments,int* num_args)
{
    char get_inp;
    int state = 0,i = 0,j = 0;
    //There are 3 states for occurance of a white space, 0th state is followed by a whitespace, 
    //1st state is followed by a '\\' which escapes ' ', 2nd state is followed by a valid character.
    int exiter = 0 ;
    while(1) 
    {
      if(exiter == 1)
            break;

        get_inp = getchar();
        switch(get_inp)
        {
            case '\n':
            {
                arguments[i][j] = '\0';
            
                if(state != 0)
                    i++;
                exiter = 1;
                break;     
            }
            case '\t':
            {
                if(state == 0)
                {
                    break;
                }
                else if(state == 2)
                {
                    arguments[i][j] = '\0';
                    j = 0;
                    i++;
                    arguments = (char **)realloc(arguments,(i + 1) * sizeof(char *));
                    arguments[i] = (char *)malloc(max_arg * sizeof(char));
                    state = 0;
                    break;
                }
                break;
            }
            case ' ':
            {
                if(state == 0)
                {
                    break;
                }
                else if(state == 1)
                {
                    arguments[i][j++] = ' ';
                    state = 2;
                    break;           
                }
                else if(state == 2)
                {
                    arguments[i][j] = '\0';
                    j = 0;
                    i++;
                    arguments = (char **)realloc(arguments,(i + 1) * sizeof(char *));
                    arguments[i] = (char *)malloc(max_arg * sizeof(char));
                    state = 0;   
                    break;
                }
                break;
            }
            case '\\':
            {
                state = 1;
                break;       
            }
            default:
            {
                arguments[i][j++] = get_inp;
                state = 2;       
            }
        }

    }
    // i contains the number of arguments
    *num_args = i;  
    return arguments;
}


// helper function 
int isNumeric(const char * s)
{
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

// clears the entire screen
void clear_screen()
{
  printf("\033[2J\033[1;1H") ;
}


// This function simply prints the environment variables
void environment()
{
   int i = 0 ;
     char *s = *environ;
     for (; s; i++) 
     {
        printf("%s\n", s);
        s = *(environ+i);
     }         
}

// This fuction changes the directory to the specified path .
void cd(const char * path) 
{
    if( chdir(path) ) 
        perror(path) ;
}

//This function returns the present directory in which we are working
//getcwd() is the function which returns the current directory
char* pwd(char* present_directory)
{
    return getcwd(present_directory,max_dir) ;
}


// We create a new directory for all the arguments 
//If mkdir() fuction fails to do so , we call perror() function .

void make_dir(int num_dir , char** dir_names)
{
    int i = 0;
    while(i < num_dir)
    {
        if(mkdir(dir_names[i],mode_MKDIR) != 0)
            perror("mkdir ") ;
       

        i++ ;
    }
}


// We remove all the directories in the argument 
//If mkdir() fuction fails to do so , we call perror() function .
void remove_dir(int num_dir , char** dir_names)
{
    int i = 0 ;
    while(i < num_dir)
    {
        if(rmdir(dir_names[i]) != 0)
            perror("rmdir ") ;
        

        i++ ;
    }   
}

// This function prints all the files and sub-directories in the present directory
void ls(int flag)
{
    DIR* directory;
    struct dirent* dir_entry;
    struct stat dir_stat;
    unsigned long int total_blocks_size;

    //If flag == 0, then we need to list all the files
    if(!flag)
    { 
        directory = opendir(".");
        if(directory == NULL)
        {
            perror("");
            return;
        }
        // hidden files start with '.' . We don't need to print them
        while((dir_entry = readdir(directory)) != NULL)
        {
            if(dir_entry->d_name[0] != '.')
                printf("%s\t",dir_entry->d_name);
            
        }   
        printf("\n");
    }
    //We have a flag -l .
    else 
    {
        directory = opendir(".");
        if(directory == NULL)
        {
            perror("");
            return;
        }
        //calculate the total block size of all the files which are not hidden
        total_blocks_size = 0;
        while((dir_entry = readdir(directory)) != NULL)
        {
            if(dir_entry->d_name[0] != '.')
            {
                if(lstat(dir_entry->d_name,&dir_stat))
                    perror("");
                
                else
                    total_blocks_size += dir_stat.st_blocks;
            }
        }
        closedir(directory);

        //open current directory for printing files with their statistics
        directory = opendir(".");
        if(directory == NULL)
        {
            perror("");
            return;
        }
        //total block size have to be multiplied by the fraction (block_sizeof_disk/block_sizeof_file) .
        total_blocks_size = (total_blocks_size * disk_blk_size)/file_blk_size;
        printf("total %lu\n",total_blocks_size);
        //print the statistics for files which are not hidden
        while((dir_entry = readdir(directory)) != NULL)
        {
            if(dir_entry->d_name[0] != '.')
            {
                //lstat used instead of stat as it gives symbolic links also in the statistics.
                if(lstat(dir_entry->d_name,&dir_stat))
                    perror("");
                
                else 
                {    
                    char permission[11] ;
                    //print the permissions of the file

                    permission[0] = (S_ISDIR(dir_stat.st_mode))  ? 'd' : '-' ;
                    permission[1] = (dir_stat.st_mode & S_IRUSR) ? 'r' : '-' ;
                    permission[2] = (dir_stat.st_mode & S_IWUSR) ? 'w' : '-' ;
                    permission[3] = (dir_stat.st_mode & S_IXUSR) ? 'x' : '-' ;
                    permission[4] = (dir_stat.st_mode & S_IRGRP) ? 'r' : '-' ; 
                    permission[5] = (dir_stat.st_mode & S_IWGRP) ? 'w' : '-' ;
                    permission[6] = (dir_stat.st_mode & S_IXGRP) ? 'x' : '-' ;
                    permission[7] = (dir_stat.st_mode & S_IROTH) ? 'r' : '-' ; 
                    permission[8] = (dir_stat.st_mode & S_IWOTH) ? 'w' : '-' ;
                    permission[9] = (dir_stat.st_mode & S_IXOTH) ? 'x' : '-' ;
                    
                    int per;
                    for(per = 0 ; per < 10 ; per++)
                        printf("%c",permission[per]) ;

                    //print number of links to this file, the user name (acc. to user id of the file),
                    //the group name (acc. to group id of the file).
                    printf(" %d %s\t%s\t",(int)dir_stat.st_nlink,getpwuid(dir_stat.st_uid)->pw_name,getgrgid(dir_stat.st_gid)->gr_name);
                    //print the size of the file.
                    printf("%5lu\t",dir_stat.st_size);   
                    time_t t = dir_stat.st_mtime;
                    struct tm lt;
                    localtime_r(&t, &lt);
                    char timbuf[80];
                    strftime(timbuf, sizeof(timbuf), "%b %d %H:%M", &lt);       
                    //print the last modified time of the file.
                    printf("%s\t%s\n",timbuf,dir_entry->d_name);
                }
            }
        }   
    }
    
    
    closedir(directory);
}


// Prints all the previous commands given by the user
// We also print the commands of the previous sessions also
void history()
{
    FILE * temp = fr ;
    //fflush(temp) ;
    int  i = 1 ;
    const size_t line_size = 300;
    char* line = malloc(line_size);
    while (fgets(line, line_size, temp) != NULL) 
     printf(" %d\t%s",i++,line);
      free(line);    // dont forget to free heap memory
}

// We print the last "kannu" number of commands 
// User gives an integer as an argument
// Those last n commands are printed out
void history_agrument(int kannu)
{
    FILE * temp = fr ;
    int  i = 1 , j = 0;
    const size_t line_size = 300;
    char* line = malloc(line_size);
    while (fgets(line, line_size, temp) != NULL) 
      j++ ;    
    rewind(temp) ;

    while (fgets(line, line_size, temp) != NULL) 
     {  
        if(i > j-kannu)
          printf(" %d\t%s",i,line);

       i++ ;

     }
      free(line);
}

// Signal handler function 
// ctrl+\ is caught using SIGQUIT and then print prev is called
void print_prev()
{
   printf("\n") ;
   char input[500] ;
   fgets(input , 500 , stdin ) ;
   //printf("%s",input) ;
    
   rewind(fr) ; 

    FILE * temp = fr ;
    int  i = 0, l;
    const size_t line_size = 300;
    char Temp[300] ;
    Temp[0] = '\0' ;
    char line[300] ;     
    rewind(temp) ;
    l = strlen(input) ;
    input[l-1] = '\0' ;
    l-- ;
     //printf("l = %d\n",l) ;
    while (fgets(line, line_size, temp) != NULL) 
    {
        int p = strlen(line) ;
        line[p-1] = '\0' ;
      for(i =0 ; i < l ; i++)
      {
        //if(input[i]== '\n') break ;
        if(input[i] == line[i])
            {   
                //printf("%c,",input[i]) ;
                continue; 
            }
        else 
            break ;
      } 
      if(i == l && i != 0) 
        {
            //printf("op") ; 
            strcpy(Temp , line) ;
            //printf("%s %d %d\n",line, i ,l);
        } 
   //printf("aayush");
   // fflush(stdin) ;
    }
    if(Temp[0] != '\0')
     printf("%s\n",Temp);
    else 
     printf("No match found\n");
 //free(line) ;
}

//Exit from the shell
void exit_cmd()
{
  exit(0) ;
}

int main()
{
  char * present_directory ;
  char *command , *new_command ; 
  char **arguments , **new_arguments ;
  char get_inp ;
  int state , status , i , j , num_arg , inp_status , is_background = 0 , pipe_count , lastpos , pipe_size;
  pipe_queue * queue;
  pid_t pid , temp_pid ;
   flag = 0; 
   struct sigaction sighandle;
   sighandle.sa_flags = SA_SIGINFO;
   sighandle.sa_sigaction = &print_prev;
   sigaction(SIGQUIT, &sighandle, NULL);   
  fp = fopen("COMMAND.txt","a") ;
  fr = fopen("COMMAND.txt","r") ;
  fflush(fp) ;
  fflush(fr) ;
 

  while(1)
  {
        temp_pid = waitpid(-1, &inp_status, WNOHANG);
    if(temp_pid > 0) 
         printf("[%d]\tDone\n" , temp_pid);

    present_directory = (char *)malloc(max_dir * sizeof(char));
     if(pwd(present_directory) != NULL)
        printf("%s>",present_directory);
        
     else
        printf(">"); 
    

    //printf("yola\n") ;
    fflush(stdin) ;

    command = (char *)malloc(max_cmd * sizeof(char));
 
    state = 0 ; i = 0 ; num_arg = 0; 
    int exiter = 0 ;
    
    while(1) 
        {
            if (exiter == 1 )
                break;
            //printf("yola") ;
            get_inp = getchar();
            //printf("ken");
            // fflush(stdin) ;
            if((int)get_inp == -1) continue ;
          //  printf("%d  ",get_inp) ;

            //printf("debug") ;
            switch(get_inp)
            {
                case '\n':
                {
                    exiter = 1;
                    break;
                }
                case '\t':
                {
                    if(state == 0)
                        break;
                    
                    else 
                    {
                        exiter = 1;
                        break;
                    }
                }
                case ' ':
                {
                    if(state == 0)
                        break;
                    
                    else
                    {
                        exiter = 1;
                        break;
                    }
                }
                case '\\':
                {
                    // state = 1;
                    break;
                }
                default:
                {
                    command[i++] = get_inp;
                    state = 1;
                    break;       
                }
            }
        }

        command[i++] = '\0';
        if(!strlen(command)) 
            continue;
        
        arguments = (char **)malloc(sizeof(char *));
        arguments[0] = (char *)malloc(max_arg * sizeof(char));
    
        if(get_inp != '\n' && get_inp != EOF) 
            arguments = generate_args(arguments,&num_arg);
        
        else 
            arguments[0][0] = '\0';
        
        fprintf(fp ,"%s " , command) ;
       // printf("command : %s\n",command) ;
        
        int k ;
        for(k = 0 ; k < num_arg ; k++)
          fprintf(fp ,"%s " , arguments[k]) ;

      //for(k = 0 ; k < num_arg ; k++)
         // printf("arguments : %s " , arguments[k]) ;
      //printf("\n") ;

        fprintf(fp , "\n") ;
        fflush(fp) ;  

        if(!strcmp(command,"cd")) 
        {
            if(strlen(arguments[0]))
                cd(arguments[0]);   
            else 
                cd(getenv("HOME"));
        }

        else if(!strcmp(command,"mkdir")) 
        {
            if(strlen(arguments[0]))
                make_dir(num_arg,arguments);
            
            else 
            {
                printf("mkdir : missing operand\n");
                continue;
            }
        }

        else if(!strcmp(command,"rmdir")) 
        {
            if(strlen(arguments[0]))
                remove_dir(num_arg,arguments);
            
            else 
            {
                printf("rmdir : missing operand\n");
                continue;
            }   
        }

        else if(!strcmp(command,"history"))
        {
            
            if (isNumeric(arguments[0]))
              {
                //printf("in if\n") ;
                rewind(fr) ;
               history_agrument(strtol(arguments[0] , &arguments[0], 10)) ;
                 //int lanu = strtol(arguments[0] , &arguments[0], 10) ;
                 //printf("%d\n", lanu) ;
               }
               
             
             else if( strlen(arguments[0]) == 0 )
               { 
                  rewind(fr)  ; 
                  history() ;
                }
            

            else
             printf("Error : Invalid agruments\n") ;
        }
            
            
        else if(!strcmp(command,"exit"))
        {
                fclose(fp) ;
                exit_cmd();
                fclose(fp) ;
        }

        else
        {
            //printf("Inside else\n") ;
          is_background = 0;
          
          i = 0 ; 
          while(i < num_arg)
            {
                if(strlen(arguments[i]) == 1)
                {
                    if(arguments[i][0] == '&')
                      {
                        //printf("Inside else odd\n") ;
                        is_background = 1;
                        num_arg = i;
                        break;
                      }
                }
                i++ ;
            }
            //printf("Inside else1\n") ;
          
          pipe_count = 0 ;
          pipe_size = num_arg ;
          
          i = 0 ;
          while(i < num_arg )
          {
            if(arguments[i][0] == '|')
                {  
                  pipe_count++ ;

                  if(pipe_count == 1) 
                     pipe_size = i ;
                }
                i++ ;
          }
          //printf("Inside else2\n") ;
          
          queue = (pipe_queue *) malloc((pipe_count+1)*sizeof(pipe_queue)) ;
          queue[0].queue_elements = (char **)malloc( pipe_size * sizeof(char *)) ;
          queue[0].queue_elements[0] = strdup(command) ;
          
          int pos ;
          lastpos = 1 , pos = 0 , i = 0 ;
          //printf("Inside else3\n") ;
         while(i < num_arg)
         { 
                if(arguments[i][0] == '|')
                {
                    
                    queue[pos].number = lastpos;
                    pos++;
                    pipe_size = 0;
                    j = i+1 ;
                    while(j < num_arg)
                    {
                        if(arguments[j][0] == '|')
                        {
                            break;
                        }
                        else 
                        {
                            pipe_size++;
                        }
                        j++;
                    }
                    if(strlen(arguments[i]) == 1)
                    {
                        lastpos = 0;    
                        queue[pos].queue_elements = (char **)malloc(pipe_size * sizeof(char *));
                    }
                    else 
                    {
                        queue[pos].queue_elements = (char **)malloc((pipe_size + 1) * sizeof(char *));
                        queue[pos].queue_elements[0] = strdup(arguments[i] + 1);
                        lastpos = 1;
                    }
                    
                }
                else 
                {
                    queue[pos].queue_elements[lastpos++] = strdup(arguments[i]);
                }
                i++ ;
            }
             // printf("Inside else4\n") ;
             queue[pos].number = lastpos ;

             int **fd ;
             fd = (int **) malloc((pipe_count+1)*sizeof(int *)) ;

             i = 0 ;

             while(i <= pipe_count)
              {
                fd[i] = (int *)malloc(2 * sizeof(int)) ;
                fd[i][0] = -1 ;
                fd[i][1] = -1 ;

                i++ ;
              }
             // printf("Inside else5\n") ;
             i = 0 ; 
            while( i <= pipe_count)
            {
               // printf("Inside for_def\n") ;
              char * input = NULL ;
              char * output = NULL ;
              int input_redirection = 0 , output_redirection = 0 ;
              int last_redirected_position = queue[i].number ;
              
              j = 0 ;
              while(j < queue[i].number)
               {
                   
                   // check for output > redirection
                    if(queue[i].queue_elements[j][0] == '>') 
                    {
                        if(strlen(queue[i].queue_elements[j]) > 1) 
                        {
                            output = strdup(queue[i].queue_elements[j] + 1);
                            output_redirection = 1;
                            if(last_redirected_position == queue[i].number ) 
                              last_redirected_position = j;
                        } 
                        else if(j + 1 < queue[i].number ) 
                        {
                            output = strdup(queue[i].queue_elements[j + 1]);
                            output_redirection = 1;
                            if(last_redirected_position == queue[i].number ) 
                              last_redirected_position = j;
                        } 
                        else 
                        {
                            fprintf(stderr, "syntax error near > no output file provided\n");
                            exit(0);
                        }
                    }

                   // printf("Inside for1\n") ;
                    if(queue[i].queue_elements[j][0] == '<') 
                    {
                        if(strlen(queue[i].queue_elements[j]) > 1) 
                        {
                            input = strdup(queue[i].queue_elements[j] + 1);
                            input_redirection = 1;
                            if(last_redirected_position == queue[i].number ) 
                              last_redirected_position = j;
                        }  

                        else if(j + 1 < queue[i].number ) 
                        {
                            input = strdup(queue[i].queue_elements[j + 1]);
                            input_redirection = 1;
                            if(last_redirected_position == queue[i].number ) 
                              last_redirected_position = j;
                        } 
                        else 
                        {
                            fprintf(stderr, "syntax error near < no input file provided\n");
                            exit(0);
                        }
                    }
                  // printf("Inside for2\n") ; 

                    j++ ;
                   // printf("Inside for3\n") ;
               }
                 //printf("Inside else6\n") ;
                if(last_redirected_position < queue[i].number) 
                  queue[i].number = last_redirected_position ;

                if(i != pipe_count && !output_redirection)
                  pipe(fd[i]) ;

                 pid = fork();

                 if(pid == 0)
                 {
                    //printf("Inside pid = 0\n");
                    if(input_redirection) 
                    {
                        
                        int fd_input ;  
                        fd_input = open(input, O_RDONLY, 0) ;
                        if(fd_input < 0) 
                        {
                            perror(input);
                            exit(0);
                        }
                        dup2(fd_input, STDIN_FILENO);
                        close(fd_input);
                    } 
                    else 
                    {
                        if(i != 0 && fd[i - 1][0] != -1) 
                        {
                            close(STDIN_FILENO);          
                            dup2(fd[i - 1][0], STDIN_FILENO);    
                            close(fd[i - 1][1]);   
                            close(fd[i - 1][0]);
                        } 
                        else if(i != 0) 
                          close(STDIN_FILENO);
                    }

                    if(output_redirection) 
                    {
                        // redirect STDOUT to the given file
                        int fd_output ;
                        fd_output = creat(output , 0644) ;
                        if(fd_output < 0) 
                        {
                            perror(output) ;
                            exit(0) ;
                        }
                        dup2(fd_output, STDOUT_FILENO);
                        close(fd_output);
                    } 
                    else 
                    {
                        if(i != pipe_count) 
                        {
                            // pipe(pipeFDs[i]);
                            close(STDOUT_FILENO);          
                            dup2(fd[i][1], STDOUT_FILENO);  
                            close(fd[i][0]);  
                            close(fd[i][1]);
                        }
                    }

                    //create a new argument list containing all the arguments.
                    new_arguments = (char **)malloc((queue[i].number + 1) * sizeof(char *));
                    
                    
                    for(j = 0; j < queue[i].number ; j++)
                        new_arguments[j] = strdup(queue[i].queue_elements[j]);                    
                    

                    //set pgid if it is a background process.
                    if(is_background) 
                        setpgid(0, 0);
                    

                    if(!strcmp(queue[i].queue_elements[0], "ls")) 
                    {
                        if(queue[i].number > 1 && !strcmp(queue[i].queue_elements[1], "-l")) 
                            ls(1);
                        
                        else 
                            ls(0);
                        
                        exit(0);
                    }

                    if(!strcmp(queue[i].queue_elements[0], "pwd")) 
                    {
                        if(pwd(present_directory) != NULL)
                            printf("%s\n",present_directory);
                        
                        else 
                            perror("");
                        exit(0);
                    }
                    //Call execvp with the filename and the arguments.
                    if(execvp(queue[i].queue_elements[0],new_arguments))
                    {
                        new_command = (char *)malloc((strlen(new_arguments[0]) + 2) * sizeof(char));
                        if(new_arguments[0][0] != '/')
                        {
                            sprintf(new_command,"./%s",new_arguments[0]);
                            if(execvp(new_command,new_arguments))
                            {
                                perror("execvp error");
                                exit(0);
                            }
                        }
                        else
                         {
                            perror("execvp error");
                            exit(0);
                        }
                    }
                }

                else if(pid > 0)
                { 
                    //printf("Inside pid >0\n");
                    temp_pid = waitpid(-1, &inp_status, WNOHANG);
                    if(temp_pid > 0) 
                        printf("[%d]\tDone\n",temp_pid);
                    
                    if(is_background) 
                        printf("[%d]\tStarted\n",pid);
                    
                    //wait if the child process is not a background process.
                    if(!is_background) 
                    {
                        wait(&status);
                        close(fd[i][1]);
                    }
                }
                else 
                {
                    perror("fork failed");
                    exit(EXIT_FAILURE); 
                }

              i++ ;
            }
          
          i = 0 ;
         while(i <= pipe_count) 
        {
                 free(fd[i]);
                 i++ ;
        }

           free(fd) ;

        }
     
  }

   return 0 ;
  
}


        





                        


