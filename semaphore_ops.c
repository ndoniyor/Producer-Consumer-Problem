#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "auxiliary_functions.h"

void down_semaphore(int semaphore_id){
    struct sembuf semop_metadata;
    semop_metadata.sem_num = 0;
    semop_metadata.sem_op = -1;     //signal down on semaphore
    semop_metadata.sem_flg = 0;
    semop(semaphore_id, &semop_metadata, 1); //wait on semaphore unless it's been signaled up
}

void up_semaphore(int semaphore_id){
    struct sembuf semop_metadata;
    semop_metadata.sem_num = 0;
    semop_metadata.sem_op = 1;      //signal up on semaphore
    semop_metadata.sem_flg = 0;
    semop(semaphore_id, &semop_metadata, 1);
}

void consumer_op(int semaphore_id, pid_t pid, char* buffer, char* filename, ssize_t* copy_data_size){
    int consumer_fd = open_error_report(filename, O_WRONLY | O_CREAT | O_EXCL, 0666);
    ssize_t current_copy_size;
	do{
      	rand_delay(1);        //wait for producer with random delay
        down_semaphore(semaphore_id); //signal down on semaphore to lock buffer
        current_copy_size = *copy_data_size;
        if(current_copy_size > 0){
            write(consumer_fd, buffer, current_copy_size);
        	kill(pid, SIGUSR2);    //send kill signal
        }
        *copy_data_size = 0;
      	up_semaphore(semaphore_id); //signal up on semaphore to unlock buffer
    }while(current_copy_size >= 0);
  
  	close(consumer_fd);  //close file
    kill(pid, SIGUSR2);  //send kill signal after file has been copied
    rand_delay(2);
    kill(pid, SIGUSR2);  //send second kill signal
}

void producer_op(int semaphore_id, pid_t pid, char* buffer, char* filename, ssize_t* copy_data_size){
    int producer_fd = open_error_report(filename, O_RDONLY, -1);

    ssize_t read_size;
    do{
    	down_semaphore(semaphore_id);   //signal down on semaphore to lock buffer
        read_size = 1;
        if(*copy_data_size == 0){
            read_size = read(producer_fd, buffer, 1024);
            if(read_size > 0)
                *copy_data_size = read_size;
            else
                *copy_data_size = -1;
            kill(pid, SIGUSR1); //send kill signal
        }  
    	up_semaphore(semaphore_id); //signal up on semaphore to unlock buffer
		rand_delay(1);
    }while(read_size > 0);
    close(producer_fd);
}