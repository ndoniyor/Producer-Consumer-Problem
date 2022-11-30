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

#define SEMAPHORE_ID 250
#define SHARED_MEM_SIZE 2048

union semun {                
    int val;
    struct semid_ds *buffer;
    ushort * array;
};

char* monitor_table;     // stores signals from program for progress tracking

void signal_handler(int signal_no)
{
    static int table_index = 0;
    int i;

    if(signal_no == SIGUSR1)    //signal from producer
        monitor_table[table_index++] = 'P'; //update monitor table
    else if(signal_no == SIGUSR2){  //signal from consumer        
        if(table_index > 0 && monitor_table[table_index - 1] == 'C') {  //two subsequent C's means the process is done
            printf("\nMonitor Table Output\n--------------------\n");   //print out monitor table
            for(i = 0; i < table_index;i++) {
                printf("%c ", monitor_table[i]);
            }
            printf("C END \n");
        }     
        monitor_table[table_index++] = 'C';
    }
    return;
}


int main(int argc, char* argv[])
{
    int semaphore_id, shared_mem_id;
    char* shared_mem_addr;
    union semun sem;  

    ssize_t* bytes_copied;  

    char* buffer;             
    struct shmid_ds shared_mem_desc;
	struct stat file_info;

    pid_t child1_pid, child2_pid, parent_pid;
    char source_file[255], dest_file[255];

    if(argc == 3) {
        strcpy(source_file, argv[1]); //assign source and destination based on command line args
        strcpy(dest_file, argv[2]); 
    }
    else {
        printf("Error: Invalid file specification\n");
        exit(1);
    }

    if (stat(source_file, &file_info) < 0) {
		perror("stat(source_file, &file_info): ");
		exit(1);
	}

    monitor_table = (char*)malloc((file_info.st_size / 512) + 10); //allocate memory for monitor table based on file size
    if(signal(SIGUSR1, signal_handler) < 0)    //create two usr signals for handling
        perror("signal(SIGUSR1, signal_handler): ");

    if(signal(SIGUSR2, signal_handler) < 0)
        perror("signal(SIGUSR2, signal_handler): ");
 
    semaphore_id = semget(SEMAPHORE_ID, 1, IPC_CREAT | 0600);   //create semaphore
    if (semaphore_id < 0) {
		perror("semget(SEMAPHORE_ID, 1, IPC_CREAT | 0600): ");
		exit(1);
    }

    sem.val = 1;      //initialize semaphore to 1
    if (semctl(semaphore_id, 0, SETVAL, sem) < 0) {
		perror("semctl(semaphore_id, 0, SETVAL, sem): ");
		exit(1);
    }

    shared_mem_id = shmget(100, SHARED_MEM_SIZE, IPC_CREAT | IPC_EXCL | 0600);  //create shared memory with 2048 bytes
    if (shared_mem_id < 0) {
        perror("shmget(100, SHARED_MEM_SIZE, IPC_CREAT | IPC_EXCL | 0600): ");
        exit(1);
    }

    shared_mem_addr = shmat(shared_mem_id, NULL, 0);    //attach shared memory to current address space
    if (!shared_mem_addr){
        perror("shmat(shared_mem_id, NULL, 0): ");
        exit(1);
    }

    /* Assign address of elements in shared memory space. */
    bytes_copied = (ssize_t*) shared_mem_addr;
    *bytes_copied = 0;
    buffer = (char*) ((void*)shared_mem_addr+sizeof(ssize_t));

    parent_pid = getpid(); 

    child1_pid = fork();        //fork child process to transfer file
    if (child1_pid == 0) {
	    producer_op(parent_pid, semaphore_id, bytes_copied, buffer, source_file);
	    exit(0);
    }
    child2_pid = fork();
    if (child2_pid == 0) {
	    consumer_op(parent_pid, semaphore_id, bytes_copied, buffer, dest_file);
	    exit(0);
    }

    int child_status;

    while((waitpid(child1_pid, &child_status, 0) < 0) && (errno == EINTR)) {}
    while((waitpid(child2_pid, &child_status, 0) < 0) && (errno == EINTR)) {}

    //cleanup
    if (shmdt(shared_mem_addr) < 0)       //detach shared memory from current address space
        perror("shmdt(shared_mem_addr): ");

    if (shmctl(shared_mem_id, IPC_RMID, &shared_mem_desc) < 0)      //deallocate shared memory
        perror("hmctl(shared_mem_id, IPC_RMID, &shared_mem_desc): ");

    return 0;   
}