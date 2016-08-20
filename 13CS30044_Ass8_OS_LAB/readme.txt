run the following commands to compile again :

gcc master.c -o master
gcc mmu.c -o mmu
gcc sch.c -o sch
gcc proc.c -o proc

//-----------------------------------------------------------------------------------

now delete all the previous shared memory , semaphores ,etc .

ipcrm -a 

//-----------------------------------------------------------------------------------

delete the Result.txt file in the folder inorder to save the results of the 
current execution of the process. Else the results will get appended to the 
file 

//-----------------------------------------------------------------------------------

./master 

The process will start to execute 
please Enter the value of k, m and f in the given range as i have specified 
the prompts on the screen will tell you about the range
this was done because my machine don't have enough space to allow for large 
data to be in shared memory
In case you wish to decide to change the range , please adjust the array
sizes accordingly as i have declared them for my predefined ranges only 

//-----------------------------------------------------------------------------------

Now , you need to close all the x-terms so that they send the signal to the main 
process whiich is waiting for them to terminate 
Only after you close all the x-terms will the master process terminate 

//-----------------------------------------------------------------------------------

Result.txt will be created in the folder

//-----------------------------------------------------------------------------------

thanks :) 