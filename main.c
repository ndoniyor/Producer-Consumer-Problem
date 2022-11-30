#include <stdio.h>	     
#include <string.h>
#include <sys/types.h>   
#include <sys/ipc.h>     
#include <sys/shm.h>	 
#include <sys/sem.h>	 
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>	 
#include <wait.h>	  
#include <stdlib.h>	 
#include <fcntl.h>
#include <errno.h>
#include "auxiliary_functions.h"
#include "semaphore_ops.h"

#define SEMAPHORE_ID 100
#define SHARED_MEM_SIZE 1024
#define ERROR -1
#define NEW_PROCESS 0

char* monitoring_table;     // stores signals from program for progress tracking

void signal_handler(int signal_no){
    static int table_index = 0;

    if(signal_no == SIGUSR1)    //signal from producer
        monitoring_table[table_index++] = 'P'; //update monitor table
    else if(signal_no == SIGUSR2){  //signal from consumer        
        if(table_index > 0){  //two subsequent C's means the process is done
            if(monitoring_table[table_index - 1] == 'C'){
                printf("-------------------\n|Monitoring Table |\n-------------------\n");   //print out monitor table
                int i;
                for(i = 0; i < table_index;i++)
                    printf("|        %c        |\n", monitoring_table[i]);
            printf("-------------------\nOperation finished. \n");
            }
        }     
        monitoring_table[table_index++] = 'C';
    }
    return;
}


int main(int argc, char* argv[]){
    union semun {       //semun declaration taken from documentation, used for semctl()
        int val;
        struct semid_ds *buf;    
        unsigned short *array;
        struct seminfo *__buf; 
    };

    union semun sem;  
    char source_file[255], dest_file[255];

    strcpy(source_file, argv[1]); //assign source and destination based on command line args
    strcpy(dest_file, argv[2]); 

    struct stat file_info;
    if(stat(source_file, &file_info) == ERROR){
		perror("stat(source_file, &file_info): ");
		exit(1);
	}

    monitoring_table = (char*)malloc((file_info.st_size / 512) + 10); //allocate memory for monitor table based on file size
    if(signal(SIGUSR1, signal_handler) == ERROR)    //create two user signals for handling
        perror("signal(SIGUSR1, signal_handler): ");

    if(signal(SIGUSR2, signal_handler) == ERROR)
        perror("signal(SIGUSR2, signal_handler): ");
 
    int semaphore_id = semget(SEMAPHORE_ID, 1, IPC_CREAT | 0600);   //create semaphore
    if(semaphore_id == ERROR){
		perror("semget(SEMAPHORE_ID, 1, IPC_CREAT | 0600): ");
		exit(1);
    }

    sem.val = 3;      //initialize semaphore to nonzero
    if(semctl(semaphore_id, 0, SETVAL, sem) == ERROR){
		perror("semctl(semaphore_id, 0, SETVAL, sem): ");
		exit(1);
    }

    int shared_mem_id = shmget(100, SHARED_MEM_SIZE, IPC_CREAT | 0660);  //create shared memory with 2048 bytes
    if(shared_mem_id == ERROR) {
        perror("shmget(100, SHARED_MEM_SIZE, IPC_CREAT | 0660): ");
        exit(1);
    }

    char* shared_mem_addr = shmat(shared_mem_id, NULL, 0);    //attach shared memory to current address space
    if(!shared_mem_addr){
        perror("shmat(shared_mem_id, NULL, 0): ");
        exit(1);
    }

    char* buffer = (char*) ((void*)shared_mem_addr + sizeof(ssize_t)); 

    pid_t parent_pid = getpid(); 

    pid_t producer_pid = fork();        //fork child process to transfer file
    
    ssize_t* copy_data_size = (ssize_t*) shared_mem_addr;      //address of shared memory assigned
    *copy_data_size = 0;

    if(producer_pid == NEW_PROCESS){
	    producer_op(semaphore_id, parent_pid, buffer, source_file, copy_data_size);
	    exit(0);    //producer operation completed successfully
    }
    pid_t consumer_pid = fork();
    if(consumer_pid == NEW_PROCESS){
	    consumer_op(semaphore_id, parent_pid, buffer, dest_file, copy_data_size);
	    exit(0);    //consumer operation completed successfully
    }

    int child_status;

    while((waitpid(producer_pid, &child_status, 0) == ERROR)) //wait for both children to finish
    while((waitpid(consumer_pid, &child_status, 0) == ERROR))

    //cleanup
    if(shmdt(shared_mem_addr) == ERROR)       //detach shared memory from current address space
        perror("shmdt(shared_mem_addr): ");

    struct shmid_ds shared_mem_desc;

    if(shmctl(shared_mem_id, IPC_RMID, &shared_mem_desc) == ERROR)      //deallocate shared memory
        perror("hmctl(shared_mem_id, IPC_RMID, &shared_mem_desc): "); 
}